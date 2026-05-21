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


void Simulation::adjustClientRequestRates(double deltaPerSecond)
{
    for (auto& node : graph_.nodes()) {
        if (NodeRegistry::generatesRequests(node.type)) {
            node.requestRatePerSecond = std::max(0.0, node.requestRatePerSecond + deltaPerSecond);
            node.baseRequestRatePerSecond = node.requestRatePerSecond;
        }
    }
    if (deltaPerSecond > 0.0) {
        PressureState delta;
        delta.backend.requestLoad = deltaPerSecond * 0.015;
        delta.network.trafficBurstiness = deltaPerSecond * 0.010;
        nudgePressureState(delta);
    }
}

void Simulation::scaleApiCapacity(double multiplier)
{
    (void)scaleApiCapacity(-1, multiplier, 3, 0.72, 1.0);
}

bool Simulation::scaleApiCapacity(int targetId, double multiplier, int maxScaleLevel, double diminishingReturn, double complexityCost)
{
    bool applied = false;
    for (auto& node : graph_.nodes()) {
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
        nudgePressureState(delta);
        addComplexity(complexityCost);
        refreshEffectiveCapacities();
    }
    return applied;
}

void Simulation::toggleCache()
{
    cacheEnabled_ = !cacheEnabled_;
    PressureState delta;
    delta.frontend.cacheEfficiency = cacheEnabled_ ? 0.14 : -0.14;
    delta.frontend.sessionPersistence = cacheEnabled_ ? 0.04 : -0.04;
    delta.backend.computeIntensity = cacheEnabled_ ? -0.03 : 0.03;
    nudgePressureState(delta);
}

void Simulation::clearCache()
{
    cacheEntries_.clear();
    PressureState delta;
    delta.frontend.cacheEfficiency = -0.08;
    delta.frontend.sessionPersistence = -0.03;
    nudgePressureState(delta);
}

void Simulation::toggleBurstMode()
{
    burstModeEnabled_ = !burstModeEnabled_;
    if (burstModeEnabled_) {
        PressureState delta;
        delta.backend.requestLoad = 0.08;
        delta.frontend.realtimeIntensity = 0.05;
        delta.network.trafficBurstiness = 0.18;
        nudgePressureState(delta);
    }
}

void Simulation::toggleRetries()
{
    scenario_.retries.enabled = !scenario_.retries.enabled;
    PressureState delta;
    delta.frontend.realtimeIntensity = scenario_.retries.enabled ? 0.04 : -0.04;
    delta.network.trafficBurstiness = scenario_.retries.enabled ? 0.04 : -0.04;
    nudgePressureState(delta);
}

void Simulation::resetProcessingCapacity()
{
    cacheEnabled_ = scenario_.cache.enabled;
    burstModeEnabled_ = scenario_.bursts.enabled;
    cacheEntries_.clear();
    for (auto& node : graph_.nodes()) {
        if (node.isProcessor()) {
            node.mechanicCapacityMultiplier = 1.0;
            node.eventCapacityMultiplier = 1.0;
            node.scaleLevel = 0;
        }
    }
    complexityScore_ = 0.0;
    pressureState_ = {};
    metrics_.setPressureState(pressureState_);
    refreshRegionSlots();
    refreshEffectiveCapacities();
}

void Simulation::setSimulationSpeed(double speed)
{
    simulationSpeed_ = std::max(0.0, speed);
    timeSystem_.setSpeed(simulationSpeed_);
    metrics_.setSimulationSpeed(simulationSpeed_);
}

void Simulation::setScenarioTrafficMultiplier(double multiplier)
{
    scenarioTrafficMultiplier_ = std::max(0.0, multiplier);
}

void Simulation::setScenarioBurst(const BurstScenario& burst)
{
    scenarioBurstOverride_ = burst;
}

void Simulation::setScenarioDatabaseCapacityMultiplier(double multiplier)
{
    scenarioDatabaseCapacityMultiplier_ = std::max(0.1, multiplier);
    for (auto& node : graph_.nodes()) {
        if (node.type == NodeType::Database) {
            node.eventCapacityMultiplier = scenarioDatabaseCapacityMultiplier_;
        }
    }
    refreshEffectiveCapacities();
}

void Simulation::setScenarioLatencyMultiplier(double multiplier)
{
    scenarioLatencyMultiplier_ = std::max(0.1, multiplier);
}

void Simulation::setScenarioDatabaseHeavyShareOverride(std::optional<double> share)
{
    scenarioDatabaseHeavyShareOverride_ = share;
}

void Simulation::setScenarioRetryDelayMultiplier(double multiplier)
{
    scenarioRetryDelayMultiplier_ = std::max(0.1, multiplier);
}

void Simulation::setLocalizedEventModifiers(std::vector<LocalizedEventModifier> modifiers)
{
    localizedEventModifiers_ = std::move(modifiers);
    refreshEffectiveCapacities();
}

bool Simulation::addRegionalDemandSource(const EventLocation& location, double requestRatePerSecond)
{
    if (location.scope != EventLocationScope::Region || location.region.empty() || requestRatePerSecond <= 0.0) {
        return false;
    }

    auto nudgeRegionalDemand = [this, requestRatePerSecond]() {
        PressureState delta;
        delta.backend.requestLoad = requestRatePerSecond * 0.018;
        delta.network.bandwidthPressure = requestRatePerSecond * 0.010;
        delta.network.trafficBurstiness = requestRatePerSecond * 0.012;
        nudgePressureState(delta);
    };

    for (auto& node : graph_.nodes()) {
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
    for (const auto& node : graph_.nodes()) {
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
    const int sourceId = graph_.addNode(std::move(node));

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
    graph_.addLink(std::move(link));
    refreshRegionSlots();
    return true;
}

void Simulation::setScenarioTime(double elapsedSeconds, double phaseElapsedSeconds, double calendarElapsedDays)
{
    timeSystem_.setScenarioElapsed(elapsedSeconds);
    timeSystem_.setPhaseElapsed(phaseElapsedSeconds);
    timeSystem_.setCalendarElapsedDays(calendarElapsedDays);
}

void Simulation::clearScenarioBurstOverride()
{
    scenarioBurstOverride_.reset();
}

void Simulation::setPaused(bool paused)
{
    timeSystem_.setPaused(paused);
}

void Simulation::setAllowedMechanics(const std::vector<MechanicType>& mechanics)
{
    allowedMechanics_.fill(false);
    if (mechanics.empty()) {
        for (const auto& definition : MechanicRegistry::definitions()) {
            const auto index = static_cast<std::size_t>(definition.type);
            if (index < allowedMechanics_.size()) {
                allowedMechanics_[index] = definition.available;
            }
        }
        return;
    }

    for (const auto mechanic : mechanics) {
        const auto index = static_cast<std::size_t>(mechanic);
        if (index < allowedMechanics_.size()) {
            allowedMechanics_[index] = true;
        }
    }
}

void Simulation::addComplexity(double amount)
{
    complexityScore_ = std::max(0.0, complexityScore_ + amount);
    metrics_.setComplexity(complexityScore_, recommendedComplexityThreshold_);
}

bool Simulation::canScaleNode(int nodeId, int maxScaleLevel) const
{
    if (nodeId >= 0) {
        const Node* node = graph_.node(nodeId);
        return node != nullptr && node->type == NodeType::ApiService && node->scaleLevel < maxScaleLevelForNode(nodeId, maxScaleLevel);
    }
    for (const auto& node : graph_.nodes()) {
        if (node.type == NodeType::ApiService && node.scaleLevel < std::max(1, maxScaleLevel)) {
            return true;
        }
    }
    return false;
}

int Simulation::scaleLevelForNode(int nodeId) const
{
    const Node* node = graph_.node(nodeId);
    return node != nullptr ? node->scaleLevel : 0;
}

int Simulation::maxScaleLevelForNode(int nodeId, int contentMaxScaleLevel) const
{
    const Node* node = graph_.node(nodeId);
    if (node == nullptr) {
        return std::max(1, contentMaxScaleLevel);
    }
    return std::max(1, contentMaxScaleLevel > 0 ? contentMaxScaleLevel : node->maxScaleLevel);
}
