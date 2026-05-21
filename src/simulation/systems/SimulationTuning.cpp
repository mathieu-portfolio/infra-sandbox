#include "simulation/systems/SimulationTuningSystem.hpp"
#include "simulation/systems/SimulationPressureSystem.hpp"
#include "simulation/systems/SimulationModifierSystem.hpp"
#include "simulation/systems/SimulationTopologySystem.hpp"

#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>

namespace {
std::optional<GeoLocation> regionCenter(const std::string& regionName)
{
    for (const auto& definition : GeographicRegistry::definitions()) {
        if (definition.name == regionName) {
            return definition.center;
        }
    }
    return std::nullopt;
}
}


void SimulationTuningSystem::adjustClientRequestRates(Simulation& simulation, double deltaPerSecond)
{
    for (auto& node : simulation.graph_.nodes()) {
        if (NodeRegistry::generatesRequests(node.type)) {
            node.requestRatePerSecond = std::max(0.0, node.requestRatePerSecond + deltaPerSecond);
            node.baseRequestRatePerSecond = node.requestRatePerSecond;
        }
    }
    if (deltaPerSecond > 0.0) {
        PressureState delta;
        delta.backend.requestLoad = deltaPerSecond * 0.015;
        delta.network.trafficBurstiness = deltaPerSecond * 0.010;
        SimulationPressureSystem::nudgePressureState(simulation, delta);
    }
}

void SimulationTuningSystem::scaleApiCapacity(Simulation& simulation, double multiplier)
{
    (void)scaleApiCapacity(simulation, -1, multiplier, 3, 0.72, 1.0);
}

bool SimulationTuningSystem::scaleApiCapacity(Simulation& simulation, int targetId, double multiplier, int maxScaleLevel, double diminishingReturn, double complexityCost)
{
    bool applied = false;
    for (auto& node : simulation.graph_.nodes()) {
        if (node.type != NodeType::ApiService) {
            continue;
        }
        if (targetId >= 0 && node.id != targetId) {
            continue;
        }
        node.maxScaleLevel = std::max(1, maxScaleLevel);
        if (node.scaleLevel >= node.maxScaleLevel) {
            continue;
        }
        const double levelEfficiency = std::pow(std::clamp(diminishingReturn, 0.1, 1.0), static_cast<double>(node.scaleLevel));
        const double effectiveMultiplier = 1.0 + (std::max(1.0, multiplier) - 1.0) * levelEfficiency;
        node.mechanicCapacityMultiplier = std::max(0.1, node.mechanicCapacityMultiplier * effectiveMultiplier);
        ++node.scaleLevel;
        applied = true;
    }
    if (applied) {
        PressureState delta;
        delta.backend.computeIntensity = -0.05;
        delta.backend.queuePressure = -0.03;
        delta.backend.serviceFragmentation = complexityCost * 0.025;
        SimulationPressureSystem::nudgePressureState(simulation, delta);
        addComplexity(simulation, complexityCost);
        SimulationModifierSystem::refreshEffectiveCapacities(simulation);
    }
    return applied;
}

void SimulationTuningSystem::toggleCache(Simulation& simulation)
{
    simulation.cacheEnabled_ = !simulation.cacheEnabled_;
    PressureState delta;
    delta.frontend.cacheEfficiency = simulation.cacheEnabled_ ? 0.14 : -0.14;
    delta.frontend.sessionPersistence = simulation.cacheEnabled_ ? 0.04 : -0.04;
    delta.backend.computeIntensity = simulation.cacheEnabled_ ? -0.03 : 0.03;
    SimulationPressureSystem::nudgePressureState(simulation, delta);
}

void SimulationTuningSystem::clearCache(Simulation& simulation)
{
    simulation.cacheEntries_.clear();
    PressureState delta;
    delta.frontend.cacheEfficiency = -0.08;
    delta.frontend.sessionPersistence = -0.03;
    SimulationPressureSystem::nudgePressureState(simulation, delta);
}

void SimulationTuningSystem::toggleBurstMode(Simulation& simulation)
{
    simulation.burstModeEnabled_ = !simulation.burstModeEnabled_;
    if (simulation.burstModeEnabled_) {
        PressureState delta;
        delta.backend.requestLoad = 0.08;
        delta.frontend.realtimeIntensity = 0.05;
        delta.network.trafficBurstiness = 0.18;
        SimulationPressureSystem::nudgePressureState(simulation, delta);
    }
}

void SimulationTuningSystem::toggleRetries(Simulation& simulation)
{
    simulation.scenario_.retries.enabled = !simulation.scenario_.retries.enabled;
    PressureState delta;
    delta.frontend.realtimeIntensity = simulation.scenario_.retries.enabled ? 0.04 : -0.04;
    delta.network.trafficBurstiness = simulation.scenario_.retries.enabled ? 0.04 : -0.04;
    SimulationPressureSystem::nudgePressureState(simulation, delta);
}

void SimulationTuningSystem::resetProcessingCapacity(Simulation& simulation)
{
    simulation.cacheEnabled_ = simulation.scenario_.cache.enabled;
    simulation.burstModeEnabled_ = simulation.scenario_.bursts.enabled;
    simulation.cacheEntries_.clear();
    for (auto& node : simulation.graph_.nodes()) {
        if (node.isProcessor()) {
            node.mechanicCapacityMultiplier = 1.0;
            node.eventCapacityMultiplier = 1.0;
            node.scaleLevel = 0;
        }
    }
    simulation.complexityScore_ = 0.0;
    simulation.pressureState_ = {};
    simulation.metrics_.setPressureState(simulation.pressureState_);
    SimulationTopologySystem::refreshRegionSlots(simulation);
    SimulationModifierSystem::refreshEffectiveCapacities(simulation);
}

void SimulationTuningSystem::setSimulationSpeed(Simulation& simulation, double speed)
{
    simulation.simulationSpeed_ = std::max(0.0, speed);
    simulation.timeSystem_.setSpeed(simulation.simulationSpeed_);
    simulation.metrics_.setSimulationSpeed(simulation.simulationSpeed_);
}

void SimulationTuningSystem::setScenarioTrafficMultiplier(Simulation& simulation, double multiplier)
{
    simulation.scenarioTrafficMultiplier_ = std::max(0.0, multiplier);
}

void SimulationTuningSystem::setScenarioBurst(Simulation& simulation, const BurstScenario& burst)
{
    simulation.scenarioBurstOverride_ = burst;
}

void SimulationTuningSystem::setScenarioDatabaseCapacityMultiplier(Simulation& simulation, double multiplier)
{
    simulation.scenarioDatabaseCapacityMultiplier_ = std::max(0.1, multiplier);
    for (auto& node : simulation.graph_.nodes()) {
        if (node.type == NodeType::Database) {
            node.eventCapacityMultiplier = simulation.scenarioDatabaseCapacityMultiplier_;
        }
    }
    SimulationModifierSystem::refreshEffectiveCapacities(simulation);
}

void SimulationTuningSystem::setScenarioLatencyMultiplier(Simulation& simulation, double multiplier)
{
    simulation.scenarioLatencyMultiplier_ = std::max(0.1, multiplier);
}

void SimulationTuningSystem::setScenarioDatabaseHeavyShareOverride(Simulation& simulation, std::optional<double> share)
{
    simulation.scenarioDatabaseHeavyShareOverride_ = share;
}

void SimulationTuningSystem::setScenarioRetryDelayMultiplier(Simulation& simulation, double multiplier)
{
    simulation.scenarioRetryDelayMultiplier_ = std::max(0.1, multiplier);
}

void SimulationTuningSystem::setLocalizedEventModifiers(Simulation& simulation, std::vector<LocalizedEventModifier> modifiers)
{
    simulation.localizedEventModifiers_ = std::move(modifiers);
    SimulationModifierSystem::refreshEffectiveCapacities(simulation);
}

bool SimulationTuningSystem::addRegionalDemandSource(Simulation& simulation, const EventLocation& location, double requestRatePerSecond)
{
    if (location.scope != EventLocationScope::Region || location.region.empty() || requestRatePerSecond <= 0.0) {
        return false;
    }

    auto nudgeRegionalDemand = [&simulation, requestRatePerSecond]() {
        PressureState delta;
        delta.backend.requestLoad = requestRatePerSecond * 0.018;
        delta.network.bandwidthPressure = requestRatePerSecond * 0.010;
        delta.network.trafficBurstiness = requestRatePerSecond * 0.012;
        SimulationPressureSystem::nudgePressureState(simulation, delta);
    };

    for (auto& node : simulation.graph_.nodes()) {
        if (node.type == NodeType::ClientCluster && node.hasGeoLocation && node.geoLocation.regionName == location.region) {
            node.requestRatePerSecond += requestRatePerSecond;
            node.baseRequestRatePerSecond = node.requestRatePerSecond;
            nudgeRegionalDemand();
            return true;
        }
    }

    const auto locationCenter = regionCenter(location.region);
    if (!locationCenter) {
        return false;
    }

    const Node* targetApi = nullptr;
    double bestDistance = std::numeric_limits<double>::max();
    for (const auto& node : simulation.graph_.nodes()) {
        if (node.type != NodeType::ApiService) {
            continue;
        }
        const double distance = node.hasGeoLocation
            ? MapProjection::greatCircleKilometers(*locationCenter, node.geoLocation)
            : 0.0;
        if (distance < bestDistance) {
            bestDistance = distance;
            targetApi = &node;
        }
    }
    if (targetApi == nullptr) {
        return false;
    }

    nudgeRegionalDemand();

    Node node;
    node.name = location.region + " users";
    node.type = NodeType::ClientCluster;
    node.geoLocation = *locationCenter;
    node.hasGeoLocation = true;
    node.position = MapProjection::projectEquirectangular(node.geoLocation);
    node.requestRatePerSecond = requestRatePerSecond;
    node.baseRequestRatePerSecond = requestRatePerSecond;
    const int sourceId = simulation.graph_.addNode(std::move(node));

    Link link;
    link.sourceNodeId = sourceId;
    link.targetNodeId = targetApi->id;
    link.baseLatencySeconds = 0.18;
    if (targetApi->hasGeoLocation) {
        const GeographicSystem geography;
        link.baseLatencySeconds = geography.latencySeconds(*locationCenter, targetApi->geoLocation, link.baseLatencySeconds);
        link.geographicDistanceKm = MapProjection::greatCircleKilometers(*locationCenter, targetApi->geoLocation);
        link.geographicLatencyContributionSeconds = std::max(0.0, link.baseLatencySeconds - 0.18);
    }
    simulation.graph_.addLink(std::move(link));
    SimulationTopologySystem::refreshRegionSlots(simulation);
    return true;
}

void SimulationTuningSystem::setScenarioTime(Simulation& simulation, double elapsedSeconds, double phaseElapsedSeconds, double calendarElapsedDays)
{
    simulation.timeSystem_.setScenarioElapsed(elapsedSeconds);
    simulation.timeSystem_.setPhaseElapsed(phaseElapsedSeconds);
    simulation.timeSystem_.setCalendarElapsedDays(calendarElapsedDays);
}

void SimulationTuningSystem::clearScenarioBurstOverride(Simulation& simulation)
{
    simulation.scenarioBurstOverride_.reset();
}

void SimulationTuningSystem::setPaused(Simulation& simulation, bool paused)
{
    simulation.timeSystem_.setPaused(paused);
}

void SimulationTuningSystem::setAllowedMechanics(Simulation& simulation, const std::vector<MechanicType>& mechanics)
{
    simulation.allowedMechanics_.fill(false);
    if (mechanics.empty()) {
        for (const auto& definition : MechanicRegistry::definitions()) {
            const auto index = static_cast<std::size_t>(definition.type);
            if (index < simulation.allowedMechanics_.size()) {
                simulation.allowedMechanics_[index] = definition.available;
            }
        }
        return;
    }

    for (const auto mechanic : mechanics) {
        const auto index = static_cast<std::size_t>(mechanic);
        if (index < simulation.allowedMechanics_.size()) {
            simulation.allowedMechanics_[index] = true;
        }
    }
}

void SimulationTuningSystem::addComplexity(Simulation& simulation, double amount)
{
    simulation.complexityScore_ = std::max(0.0, simulation.complexityScore_ + amount);
    simulation.metrics_.setComplexity(simulation.complexityScore_, simulation.recommendedComplexityThreshold_);
}

bool SimulationTuningSystem::canScaleNode(const Simulation& simulation, int nodeId, int maxScaleLevel)
{
    if (nodeId >= 0) {
        const Node* node = simulation.graph_.node(nodeId);
        return node != nullptr && node->type == NodeType::ApiService && node->scaleLevel < maxScaleLevelForNode(simulation, nodeId, maxScaleLevel);
    }
    for (const auto& node : simulation.graph_.nodes()) {
        if (node.type == NodeType::ApiService && node.scaleLevel < std::max(1, maxScaleLevel)) {
            return true;
        }
    }
    return false;
}

int SimulationTuningSystem::scaleLevelForNode(const Simulation& simulation, int nodeId)
{
    const Node* node = simulation.graph_.node(nodeId);
    return node != nullptr ? node->scaleLevel : 0;
}

int SimulationTuningSystem::maxScaleLevelForNode(const Simulation& simulation, int nodeId, int contentMaxScaleLevel)
{
    const Node* node = simulation.graph_.node(nodeId);
    if (node == nullptr) {
        return std::max(1, contentMaxScaleLevel);
    }
    return std::max(1, contentMaxScaleLevel > 0 ? contentMaxScaleLevel : node->maxScaleLevel);
}
