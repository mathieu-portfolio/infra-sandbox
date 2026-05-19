#include "gameplay/Event.hpp"

#include "simulation/Simulation.hpp"

#include <algorithm>
#include <functional>
#include <set>
#include <utility>

void EventManager::reset(std::vector<EventDefinition> definitions, std::uint32_t seed)
{
    definitions_ = std::move(definitions);
    seed_ = seed;
    activeEvents_.clear();
    pendingEvents_.clear();
    fired_.assign(definitions_.size(), false);
    recentEvents_.clear();
}

void EventManager::update(double dt, double scenarioTimeSeconds, int phaseIndex, const Simulation& simulation)
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
        if (scenarioTimeSeconds >= it->fireAtSeconds) {
            activate(it->definitionIndex, scenarioTimeSeconds, simulation);
            it = pendingEvents_.erase(it);
        } else {
            ++it;
        }
    }

    for (std::size_t i = 0; i < definitions_.size(); ++i) {
        if (fired_[i] && !definitions_[i].repeatable) {
            continue;
        }
        if (triggerMet(definitions_[i], scenarioTimeSeconds, phaseIndex, simulation)) {
            fired_[i] = true;
            if (definitions_[i].trigger.delaySeconds > 0.0) {
                pendingEvents_.push_back({i, scenarioTimeSeconds + definitions_[i].trigger.delaySeconds});
            } else {
                activate(i, scenarioTimeSeconds, simulation);
            }
        }
    }
}

void EventManager::inject(EventDefinition definition, double scenarioTimeSeconds, const Simulation& simulation)
{
    definitions_.push_back(std::move(definition));
    fired_.push_back(true);
    activate(definitions_.size() - 1, scenarioTimeSeconds, simulation);
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

bool EventManager::triggerMet(const EventDefinition& definition, double scenarioTimeSeconds, int phaseIndex, const Simulation& simulation) const
{
    const auto& trigger = definition.trigger;
    switch (trigger.type) {
    case EventTriggerType::TimeBased:
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

void EventManager::activate(std::size_t definitionIndex, double scenarioTimeSeconds, const Simulation& simulation)
{
    if (definitionIndex >= definitions_.size()) {
        return;
    }

    EventDefinition definition = definitions_[definitionIndex];
    definition.location = resolvedLocation(definition, scenarioTimeSeconds, simulation);
    activeEvents_.push_back({
        .definition = definition,
        .startedAtSeconds = scenarioTimeSeconds,
        .remainingSeconds = definition.durationSeconds,
    });
    recentEvents_.push_back({
        .timeSeconds = scenarioTimeSeconds,
        .category = definition.category,
        .name = definition.name,
        .locationLabel = eventLocationLabel(definition.location),
    });

    while (recentEvents_.size() > 10) {
        recentEvents_.pop_front();
    }
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
