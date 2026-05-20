#include "simulation/core/Simulation.hpp"

#include "simulation/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


void Simulation::adjustClientRequestRates(double deltaPerSecond)
{
    for (auto& node : graph_.nodes()) {
        if (NodeRegistry::generatesRequests(node.type)) {
            node.requestRatePerSecond = std::max(0.0, node.requestRatePerSecond + deltaPerSecond);
            node.baseRequestRatePerSecond = node.requestRatePerSecond;
        }
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
        addComplexity(complexityCost);
        refreshEffectiveCapacities();
    }
    return applied;
}

void Simulation::toggleCache()
{
    cacheEnabled_ = !cacheEnabled_;
}

void Simulation::clearCache()
{
    cacheEntries_.clear();
}

void Simulation::toggleBurstMode()
{
    burstModeEnabled_ = !burstModeEnabled_;
}

void Simulation::toggleRetries()
{
    scenario_.retries.enabled = !scenario_.retries.enabled;
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

