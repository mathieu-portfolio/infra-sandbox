#pragma once

enum class ProgressionTier {
    Foundations,
    LocalScale,
    StateAndCache,
    FailureFeedback,
    GeographicScale,
    DistributedSystems,
    Complexity
};

enum class ScenarioArchetype {
    FirstRequest,
    LocalStartup,
    DatabasePressure,
    BurstTraffic,
    TransatlanticLatency,
    GlobalReadPlatform
};

enum class ScenarioModifierType {
    MobileRefreshWave,
    ReadHeavyBehavior,
    AggressiveRetries,
    RegionalTrafficSpike,
    SlowDatabaseWindow
};

enum class ScenarioRunState {
    Running,
    Succeeded,
    RecoverableFailure
};

enum class EngineeringDomain {
    Frontend,
    Backend,
    Infrastructure,
    Data,
    Operations,
    Count
};

enum class GameplayDurationUnit {
    Turns,
    Seconds,
    Minutes,
    Days,
    Months,
    Years
};

enum class ObjectiveConditionType {
    SurviveDuration,
    PressureDetected,
    PressureBelow,
    MetricBelow,
    ActionUsed
};

enum class ObjectiveRewardType {
    UnlockIntervention,
    UnlockMetric,
    UnlockOverlay,
    UnlockScenarioPhase,
    UnlockScenario,
    EmitFeedback,
    CompleteScenario
};

enum class EducationalFocus {
    Queues,
    Caching,
    Scaling,
    Reliability,
    Persistence,
    Latency,
    Geography
};

enum class TrafficProfileType {
    Constant,
    Bursty,
    PeriodicSpikes,
    GradualGrowth
};

enum class ScenarioObjectiveType {
    MaxLatency,
    MaxErrorRate,
    MinThroughput,
    SurviveDuration,
    StabilizeQueues
};
