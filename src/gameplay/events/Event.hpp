#pragma once

#include "core/validation/ValueSpec.hpp"
#include "core/simulation/Mechanics.hpp"
#include "core/topology/NodeDefinition.hpp"
#include "simulation/metrics/Metrics.hpp"
#include "simulation/metrics/PressureAnalysis.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

class Simulation;


enum class EventCategory {
    TrafficEvent,
    InfrastructureEvent,
    ReliabilityEvent,
    GeographicEvent,
    DemandEvent,
    FailureEvent,
    RecoveryEvent,
    EducationalEvent
};

enum class EventMoment {
    PlanningStart,
    Simulation
};

enum class EventTriggerType {
    TimeBased,
    MetricThreshold,
    PressureThreshold,
    ScenarioPhase
};

enum class EventMetric {
    AverageLatency,
    TimeoutRate,
    RetryRate,
    DatabaseQueue,
    ApiQueue,
    CacheHitRate
};

enum class EventEffectType {
    TrafficSpike,
    ViralGrowth,
    DatabaseSlowdown,
    RetryStorm,
    CacheWarmup,
    PartialRecovery,
    MechanicUnlock,
    RegionalDemand
};

enum class EventLocationScope {
    Global,
    Region,
    NodeType,
    RandomRegion
};

struct EventLocation {
    EventLocationScope scope = EventLocationScope::Global;
    std::string region;
    std::vector<std::string> candidateRegions;
    NodeType nodeType = NodeType::ApiService;
};

struct EventTrigger {
    EventTriggerType type = EventTriggerType::TimeBased;
    double timeSeconds = 0.0;
    NumericRange timeSecondsRange{0.0, 0.0};
    int turnNumber = 0;
    EventMetric metric = EventMetric::AverageLatency;
    PressureCategory pressure = PressureCategory::None;
    double threshold = 0.0;
    int phaseIndex = -1;
    double delaySeconds = 0.0;
    NumericRange delaySecondsRange{0.0, 0.0};
    int delayTurns = 0;
};

struct EventEffect {
    EventEffectType type = EventEffectType::TrafficSpike;
    double trafficMultiplier = 1.0;
    NumericRange trafficMultiplierRange{1.0, 1.0};
    double burstMultiplier = 1.0;
    NumericRange burstMultiplierRange{1.0, 1.0};
    double databaseCapacityMultiplier = 1.0;
    NumericRange databaseCapacityMultiplierRange{1.0, 1.0};
    double latencyMultiplier = 1.0;
    NumericRange latencyMultiplierRange{1.0, 1.0};
    double retryDelayMultiplier = 1.0;
    NumericRange retryDelayMultiplierRange{1.0, 1.0};
    std::optional<double> databaseHeavyShare;
    std::optional<NumericRange> databaseHeavyShareRange;
    double regionalDemandRatePerSecond = 0.0;
    NumericRange regionalDemandRatePerSecondRange{0.0, 0.0};
    std::vector<MechanicType> unlockMechanics;
    PressureState pressureEffect;
    std::vector<PressureContextSignal> pressureSignals;
};

struct EventDefinition {
    std::string id;
    std::string displayName;
    std::vector<std::string> tags;
    std::string name;
    std::string description;
    EventCategory category = EventCategory::TrafficEvent;
    EventMoment moment = EventMoment::Simulation;
    EventLocation location;
    EventTrigger trigger;
    EventEffect effect;
    double durationSeconds = 10.0;
    NumericRange durationSecondsRange{10.0, 10.0};
    int durationTurns = 0;
    double intensity = 1.0;
    NumericRange intensityRange{1.0, 1.0};
    double weight = 1.0;
    bool repeatable = false;
};

struct ActiveEvent {
    EventDefinition definition;
    double startedAtSeconds = 0.0;
    int startedAtTurn = 0;
    double remainingSeconds = 0.0;
};

struct EventModifiers {
    double trafficMultiplier = 1.0;
    double burstMultiplier = 1.0;
    double databaseCapacityMultiplier = 1.0;
    double latencyMultiplier = 1.0;
    double retryDelayMultiplier = 1.0;
    std::optional<double> databaseHeavyShare;
    std::vector<MechanicType> unlockedMechanics;
    PressureState pressureEffect;
    std::vector<PressureContextSignal> pressureSignals;
};

struct LocalizedEventModifier {
    EventLocation location;
    EventEffect effect;
};

struct EventLogEntry {
    double timeSeconds = 0.0;
    int turnNumber = 0;
    EventCategory category = EventCategory::TrafficEvent;
    std::string name;
    std::string description;
    std::string locationLabel;
};

class EventManager {
public:
    void reset(std::vector<EventDefinition> definitions, std::uint32_t seed = 0);
    void update(double dt, double scenarioTimeSeconds, int turnNumber, double secondsPerTurn, int phaseIndex, Simulation& simulation);
    void inject(EventDefinition definition, double scenarioTimeSeconds, int turnNumber, double secondsPerTurn, Simulation& simulation);
    [[nodiscard]] std::optional<EventLogEntry> rollPlanningEvent(double scenarioTimeSeconds, int turnNumber, double secondsPerTurn, int phaseIndex, Simulation& simulation);
    [[nodiscard]] std::vector<EventLogEntry> eventsSince(std::size_t startIndex) const;
    [[nodiscard]] std::size_t recentEventCount() const;
    void clear();

    [[nodiscard]] const std::vector<ActiveEvent>& activeEvents() const;
    [[nodiscard]] const std::deque<EventLogEntry>& recentEvents() const;
    [[nodiscard]] EventModifiers modifiers() const;
    [[nodiscard]] std::vector<LocalizedEventModifier> localizedModifiers() const;
    [[nodiscard]] std::string latestEventName() const;

private:
    struct PendingEvent {
        std::size_t definitionIndex = 0;
        double fireAtSeconds = 0.0;
        int fireAtTurn = 0;
    };

    [[nodiscard]] bool triggerMet(const EventDefinition& definition, double scenarioTimeSeconds, int turnNumber, int phaseIndex, const Simulation& simulation) const;
    [[nodiscard]] bool eligibleForRoll(std::size_t definitionIndex, EventMoment moment, double scenarioTimeSeconds, int turnNumber, int phaseIndex, const Simulation& simulation) const;
    [[nodiscard]] double metricValue(EventMetric metric, const Simulation& simulation) const;
    EventLogEntry activate(std::size_t definitionIndex, double scenarioTimeSeconds, int turnNumber, double secondsPerTurn, Simulation& simulation);
    [[nodiscard]] EventLocation resolvedLocation(const EventDefinition& definition, double scenarioTimeSeconds, const Simulation& simulation) const;

    std::vector<EventDefinition> definitions_;
    std::vector<ActiveEvent> activeEvents_;
    std::vector<PendingEvent> pendingEvents_;
    std::vector<bool> fired_;
    std::deque<EventLogEntry> recentEvents_;
    std::uint32_t seed_ = 0;
};

const char* eventCategoryName(EventCategory category);
std::string eventLocationLabel(const EventLocation& location);
