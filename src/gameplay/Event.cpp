#include "gameplay/Event.hpp"

#include "simulation/Simulation.hpp"

#include <algorithm>
#include <utility>

void EventManager::reset(std::vector<EventDefinition> definitions)
{
    definitions_ = std::move(definitions);
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
            activate(it->definitionIndex, scenarioTimeSeconds);
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
                activate(i, scenarioTimeSeconds);
            }
        }
    }
}

void EventManager::inject(EventDefinition definition, double scenarioTimeSeconds)
{
    definitions_.push_back(std::move(definition));
    fired_.push_back(true);
    activate(definitions_.size() - 1, scenarioTimeSeconds);
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
        if (active.definition == nullptr) {
            continue;
        }
        const auto& effect = active.definition->effect;
        modifiers.trafficMultiplier *= effect.trafficMultiplier;
        modifiers.burstMultiplier *= effect.burstMultiplier;
        modifiers.databaseCapacityMultiplier *= effect.databaseCapacityMultiplier;
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

void EventManager::activate(std::size_t definitionIndex, double scenarioTimeSeconds)
{
    if (definitionIndex >= definitions_.size()) {
        return;
    }

    const auto& definition = definitions_[definitionIndex];
    activeEvents_.push_back({
        .definition = &definition,
        .startedAtSeconds = scenarioTimeSeconds,
        .remainingSeconds = definition.durationSeconds,
    });
    recentEvents_.push_back({
        .timeSeconds = scenarioTimeSeconds,
        .category = definition.category,
        .name = definition.name,
    });

    while (recentEvents_.size() > 10) {
        recentEvents_.pop_front();
    }
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
