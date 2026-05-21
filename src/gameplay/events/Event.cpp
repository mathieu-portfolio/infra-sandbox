#include "gameplay/events/Event.hpp"

#include "simulation/core/Simulation.hpp"

#include <algorithm>
#include <functional>
#include <random>
#include <set>
#include <utility>

namespace {
void addPressureState(PressureState& target, const PressureState& source)
{
    target.frontend.assetWeight += source.frontend.assetWeight;
    target.frontend.renderComplexity += source.frontend.renderComplexity;
    target.frontend.cacheEfficiency += source.frontend.cacheEfficiency;
    target.frontend.realtimeIntensity += source.frontend.realtimeIntensity;
    target.frontend.sessionPersistence += source.frontend.sessionPersistence;
    target.frontend.mobileCompatibility += source.frontend.mobileCompatibility;
    target.backend.requestLoad += source.backend.requestLoad;
    target.backend.queuePressure += source.backend.queuePressure;
    target.backend.computeIntensity += source.backend.computeIntensity;
    target.backend.serviceFragmentation += source.backend.serviceFragmentation;
    target.network.bandwidthPressure += source.network.bandwidthPressure;
    target.network.latencySensitivity += source.network.latencySensitivity;
    target.network.trafficBurstiness += source.network.trafficBurstiness;
}
}

void EventManager::reset(std::vector<EventDefinition> definitions, std::uint32_t seed)
{
    definitions_ = std::move(definitions);
    seed_ = seed;
    activeEvents_.clear();
    pendingEvents_.clear();
    fired_.assign(definitions_.size(), false);
    recentEvents_.clear();
}

void EventManager::update(double dt, double scenarioTimeSeconds, int turnNumber, double secondsPerTurn, int phaseIndex, Simulation& simulation)
{
    for (auto& active : activeEvents_) {
        active.remainingSeconds -= dt;
    }
    activeEvents_.erase(
        std::remove_if(activeEvents_.begin(), activeEvents_.end(), [](const ActiveEvent& event) {
            return event.remainingSeconds <= 0.0;
        }),
        activeEvents_.end());

    for (auto it = pendingEvents_.begin(); it != pendingEvents_.end();) {
        if ((it->fireAtTurn > 0 && turnNumber >= it->fireAtTurn) || (it->fireAtTurn <= 0 && scenarioTimeSeconds >= it->fireAtSeconds)) {
            activate(it->definitionIndex, scenarioTimeSeconds, turnNumber, secondsPerTurn, simulation);
            it = pendingEvents_.erase(it);
        } else {
            ++it;
        }
    }

    for (std::size_t i = 0; i < definitions_.size(); ++i) {
        if (definitions_[i].moment != EventMoment::Simulation) {
            continue;
        }
        if (fired_[i] && !definitions_[i].repeatable) {
            continue;
        }
        if (triggerMet(definitions_[i], scenarioTimeSeconds, turnNumber, phaseIndex, simulation)) {
            fired_[i] = true;
            if (definitions_[i].trigger.delayTurns > 0) {
                pendingEvents_.push_back({.definitionIndex = i, .fireAtSeconds = 0.0, .fireAtTurn = turnNumber + definitions_[i].trigger.delayTurns});
            } else if (definitions_[i].trigger.delaySeconds > 0.0) {
                pendingEvents_.push_back({.definitionIndex = i, .fireAtSeconds = scenarioTimeSeconds + definitions_[i].trigger.delaySeconds, .fireAtTurn = 0});
            } else {
                activate(i, scenarioTimeSeconds, turnNumber, secondsPerTurn, simulation);
            }
        }
    }
}

void EventManager::inject(EventDefinition definition, double scenarioTimeSeconds, int turnNumber, double secondsPerTurn, Simulation& simulation)
{
    definitions_.push_back(std::move(definition));
    fired_.push_back(true);
    activate(definitions_.size() - 1, scenarioTimeSeconds, turnNumber, secondsPerTurn, simulation);
}

std::optional<EventLogEntry> EventManager::rollPlanningEvent(double scenarioTimeSeconds, int turnNumber, double secondsPerTurn, int phaseIndex, Simulation& simulation)
{
    std::vector<std::size_t> eligible;
    double totalWeight = 0.0;
    for (std::size_t i = 0; i < definitions_.size(); ++i) {
        if (!eligibleForRoll(i, EventMoment::PlanningStart, scenarioTimeSeconds, turnNumber, phaseIndex, simulation)) {
            continue;
        }
        eligible.push_back(i);
        totalWeight += std::max(0.0, definitions_[i].weight);
    }

    if (eligible.empty() || totalWeight <= 0.0) {
        return std::nullopt;
    }

    const std::size_t turnSalt = static_cast<std::size_t>(turnNumber * 1009);
    std::mt19937 rng(seed_ ^ static_cast<std::uint32_t>(turnSalt + recentEvents_.size() * 7919U));
    std::uniform_real_distribution<double> distribution(0.0, totalWeight);
    double pick = distribution(rng);
    for (const std::size_t index : eligible) {
        pick -= std::max(0.0, definitions_[index].weight);
        if (pick <= 0.0) {
            fired_[index] = true;
            return activate(index, scenarioTimeSeconds, turnNumber, secondsPerTurn, simulation);
        }
    }

    const std::size_t fallback = eligible.back();
    fired_[fallback] = true;
    return activate(fallback, scenarioTimeSeconds, turnNumber, secondsPerTurn, simulation);
}

std::vector<EventLogEntry> EventManager::eventsSince(std::size_t startIndex) const
{
    std::vector<EventLogEntry> result;
    if (startIndex >= recentEvents_.size()) {
        return result;
    }
    for (std::size_t i = startIndex; i < recentEvents_.size(); ++i) {
        result.push_back(recentEvents_[i]);
    }
    return result;
}

std::size_t EventManager::recentEventCount() const
{
    return recentEvents_.size();
}

void EventManager::clear()
{
    activeEvents_.clear();
    pendingEvents_.clear();
    recentEvents_.clear();
}

const std::vector<ActiveEvent>& EventManager::activeEvents() const
{
    return activeEvents_;
}

const std::deque<EventLogEntry>& EventManager::recentEvents() const
{
    return recentEvents_;
}

EventModifiers EventManager::modifiers() const
{
    EventModifiers modifiers;
    for (const auto& active : activeEvents_) {
        if (active.definition.location.scope != EventLocationScope::Global) {
            continue;
        }
        const auto& effect = active.definition.effect;
        modifiers.trafficMultiplier *= effect.trafficMultiplier;
        modifiers.burstMultiplier *= effect.burstMultiplier;
        modifiers.databaseCapacityMultiplier *= effect.databaseCapacityMultiplier;
        modifiers.latencyMultiplier *= effect.latencyMultiplier;
        modifiers.retryDelayMultiplier *= effect.retryDelayMultiplier;
        if (effect.databaseHeavyShare) {
            modifiers.databaseHeavyShare = effect.databaseHeavyShare;
        }
        modifiers.unlockedMechanics.insert(
            modifiers.unlockedMechanics.end(),
            effect.unlockMechanics.begin(),
            effect.unlockMechanics.end());
        addPressureState(modifiers.pressureEffect, effect.pressureEffect);
        modifiers.pressureSignals.insert(
            modifiers.pressureSignals.end(),
            effect.pressureSignals.begin(),
            effect.pressureSignals.end());
    }
    return modifiers;
}

std::vector<LocalizedEventModifier> EventManager::localizedModifiers() const
{
    std::vector<LocalizedEventModifier> modifiers;
    for (const auto& active : activeEvents_) {
        if (active.definition.location.scope == EventLocationScope::Global) {
            continue;
        }
        modifiers.push_back({
            .location = active.definition.location,
            .effect = active.definition.effect,
        });
    }
    return modifiers;
}

std::string EventManager::latestEventName() const
{
    if (recentEvents_.empty()) {
        return "None";
    }
    return recentEvents_.back().name;
}

bool EventManager::triggerMet(const EventDefinition& definition, double scenarioTimeSeconds, int turnNumber, int phaseIndex, const Simulation& simulation) const
{
    const auto& trigger = definition.trigger;
    switch (trigger.type) {
    case EventTriggerType::TimeBased:
        if (trigger.turnNumber > 0) {
            return turnNumber >= trigger.turnNumber;
        }
        return scenarioTimeSeconds >= trigger.timeSeconds;
    case EventTriggerType::MetricThreshold:
        return metricValue(trigger.metric, simulation) >= trigger.threshold;
    case EventTriggerType::PressureThreshold:
        return simulation.pressure().dominantPressure == trigger.pressure;
    case EventTriggerType::ScenarioPhase:
        return phaseIndex == trigger.phaseIndex;
    }
    return false;
}


bool EventManager::eligibleForRoll(std::size_t definitionIndex, EventMoment moment, double scenarioTimeSeconds, int turnNumber, int phaseIndex, const Simulation& simulation) const
{
    if (definitionIndex >= definitions_.size()) {
        return false;
    }
    const EventDefinition& definition = definitions_[definitionIndex];
    if (definition.moment != moment) {
        return false;
    }
    if (fired_[definitionIndex] && !definition.repeatable) {
        return false;
    }
    return triggerMet(definition, scenarioTimeSeconds, turnNumber, phaseIndex, simulation);
}

double EventManager::metricValue(EventMetric metric, const Simulation& simulation) const
{
    const auto& metrics = simulation.metrics();
    switch (metric) {
    case EventMetric::AverageLatency:
        return metrics.averageLatencySeconds;
    case EventMetric::TimeoutRate:
        return metrics.timeoutRatePerSecond;
    case EventMetric::RetryRate:
        return metrics.retryRatePerSecond;
    case EventMetric::DatabaseQueue:
        return static_cast<double>(metrics.databaseQueueDepth);
    case EventMetric::ApiQueue:
        return static_cast<double>(metrics.apiQueueDepth);
    case EventMetric::CacheHitRate:
        return metrics.cacheHitRate;
    }
    return 0.0;
}

EventLogEntry EventManager::activate(std::size_t definitionIndex, double scenarioTimeSeconds, int turnNumber, double secondsPerTurn, Simulation& simulation)
{
    if (definitionIndex >= definitions_.size()) {
        return {};
    }

    EventDefinition definition = definitions_[definitionIndex];
    definition.location = resolvedLocation(definition, scenarioTimeSeconds, simulation);
    if (definition.durationTurns > 0) {
        definition.durationSeconds = std::max(1.0, secondsPerTurn) * static_cast<double>(definition.durationTurns);
    }
    if (definition.effect.type == EventEffectType::RegionalDemand) {
        simulation.addRegionalDemandSource(definition.location, definition.effect.regionalDemandRatePerSecond);
    }
    activeEvents_.push_back({
        .definition = definition,
        .startedAtSeconds = scenarioTimeSeconds,
        .startedAtTurn = turnNumber,
        .remainingSeconds = definition.durationSeconds,
    });

    EventLogEntry entry{
        .timeSeconds = scenarioTimeSeconds,
        .turnNumber = turnNumber,
        .category = definition.category,
        .name = definition.name.empty() ? definition.displayName : definition.name,
        .description = definition.description,
        .locationLabel = eventLocationLabel(definition.location),
    };
    recentEvents_.push_back(entry);

    while (recentEvents_.size() > 10) {
        recentEvents_.pop_front();
    }
    return entry;
}

EventLocation EventManager::resolvedLocation(const EventDefinition& definition, double scenarioTimeSeconds, const Simulation& simulation) const
{
    if (definition.location.scope != EventLocationScope::RandomRegion) {
        return definition.location;
    }

    std::vector<std::string> regions = definition.location.candidateRegions;
    if (regions.empty()) {
        std::set<std::string> uniqueRegions;
        for (const auto& node : simulation.graph().nodes()) {
            if (node.hasGeoLocation && !node.geoLocation.regionName.empty()) {
                uniqueRegions.insert(node.geoLocation.regionName);
            }
        }
        regions.assign(uniqueRegions.begin(), uniqueRegions.end());
    }
    if (regions.empty()) {
        return {.scope = EventLocationScope::Global};
    }

    const std::size_t seed = std::hash<std::string>{}(definition.id)
        ^ (static_cast<std::size_t>(seed_) * 0x9e3779b97f4a7c15ULL)
        ^ (static_cast<std::size_t>(scenarioTimeSeconds * 1000.0) + 0x9e3779b97f4a7c15ULL);
    return {
        .scope = EventLocationScope::Region,
        .region = regions[seed % regions.size()],
    };
}

std::string eventLocationLabel(const EventLocation& location)
{
    switch (location.scope) {
    case EventLocationScope::Global:
        return "Global";
    case EventLocationScope::Region:
        return location.region.empty() ? "Region" : location.region;
    case EventLocationScope::NodeType:
        return std::string("All ") + std::string(NodeRegistry::definition(location.nodeType).displayName);
    case EventLocationScope::RandomRegion:
        return "Random region";
    }
    return "Global";
}

const char* eventCategoryName(EventCategory category)
{
    switch (category) {
    case EventCategory::TrafficEvent:
        return "Traffic";
    case EventCategory::InfrastructureEvent:
        return "Infrastructure";
    case EventCategory::ReliabilityEvent:
        return "Reliability";
    case EventCategory::GeographicEvent:
        return "Geographic";
    case EventCategory::DemandEvent:
        return "Demand";
    case EventCategory::FailureEvent:
        return "Failure";
    case EventCategory::RecoveryEvent:
        return "Recovery";
    case EventCategory::EducationalEvent:
        return "Educational";
    }
    return "Unknown";
}
