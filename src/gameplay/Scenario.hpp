#pragma once

#include "gameplay/Event.hpp"
#include "simulation/Mechanics.hpp"
#include "simulation/Geography.hpp"
#include "simulation/InfrastructureGraph.hpp"
#include "simulation/PressureAnalysis.hpp"

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

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

struct EngineeringCapacity {
    int frontend = 1;
    int backend = 2;
    int infrastructure = 1;
    int data = 1;
    int operations = 1;
    int total = 4;
};

struct EngineeringCost {
    EngineeringDomain domain = EngineeringDomain::Backend;
    int amount = 0;
};

enum class GameplayDurationUnit {
    Seconds,
    Minutes,
    Days,
    Months,
    Years
};

struct GameplayDuration {
    double value = 90.0;
    GameplayDurationUnit unit = GameplayDurationUnit::Seconds;
    std::string label = "90 seconds of platform evolution";
    double simulationSeconds = 90.0;
    bool advancesCalendar = true;
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

struct ObjectiveReward {
    ObjectiveRewardType type = ObjectiveRewardType::EmitFeedback;
    std::string id;
    std::string message;
};

struct ProgressionTierDefinition {
    std::string id;
    std::string displayName;
    std::string description;
    std::vector<std::string> tags;
    ProgressionTier tier = ProgressionTier::Foundations;
    std::string name;
    std::vector<std::string> visibleMetrics;
    std::vector<MechanicType> availableMechanics;
    std::vector<PressureCategory> allowedPressures;
    std::vector<NodeType> allowedNodeTypes;
};

struct NodeScenario {
    std::string id;
    std::string name;
    NodeType type = NodeType::ApiService;
    Vec2 position{};
    std::optional<GeoLocation> geoLocation;
    NetworkIdentity networkIdentity;
    double requestRatePerSecond = 0.0;
    double processingCapacityPerSecond = 0.0;
    double timeoutSeconds = 6.0;
};

struct RequestTypeScenario {
    double lightweightShare = 0.68;
    double apiCostLightweight = 0.6;
    double apiCostDatabaseHeavy = 0.9;
    double apiCostReturn = 0.45;
    double databaseCostHeavy = 1.35;
    double databaseHeavyCacheableShare = 0.65;
    int cacheKeySpace = 12;
};

struct CacheScenario {
    bool enabled = false;
    int maxEntries = 8;
    double ttlSeconds = 12.0;
};

struct RetryScenario {
    bool enabled = true;
    int maxRetries = 1;
    double retryDelaySeconds = 0.75;
};

struct BurstScenario {
    bool enabled = false;
    double multiplier = 2.2;
    NumericRange multiplierRange{2.2, 2.2};
    double periodSeconds = 12.0;
    NumericRange periodSecondsRange{12.0, 12.0};
    double durationSeconds = 3.0;
    NumericRange durationSecondsRange{3.0, 3.0};
};

struct ScenarioModifierDefinition {
    std::string id;
    std::string displayName;
    std::vector<std::string> tags;
    ScenarioModifierType type = ScenarioModifierType::MobileRefreshWave;
    std::string name;
    std::string description;
    double selectionWeight = 1.0;
    NumericRange selectionWeightRange{1.0, 1.0};
    double trafficMultiplier = 1.0;
    NumericRange trafficMultiplierRange{1.0, 1.0};
    std::optional<double> databaseHeavyShare;
    std::optional<NumericRange> databaseHeavyShareRange;
    std::optional<BurstScenario> burstOverride;
    std::vector<EventDefinition> events;
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

struct TrafficProfile {
    std::string id;
    std::string displayName;
    std::string description;
    std::vector<std::string> tags;
    std::string name = "Constant";
    TrafficProfileType type = TrafficProfileType::Constant;
    double baseMultiplier = 1.0;
    NumericRange baseMultiplierRange{1.0, 1.0};
    double growthPerSecond = 0.0;
    NumericRange growthPerSecondRange{0.0, 0.0};
};

enum class ScenarioObjectiveType {
    MaxLatency,
    MaxErrorRate,
    MinThroughput,
    SurviveDuration,
    StabilizeQueues
};

struct ScenarioObjective {
    std::string id;
    std::string displayName;
    std::string description;
    std::vector<std::string> tags;
    ScenarioObjectiveType type = ScenarioObjectiveType::SurviveDuration;
    ObjectiveConditionType conditionType = ObjectiveConditionType::SurviveDuration;
    std::string conditionMetric;
    PressureCategory pressure = PressureCategory::None;
    std::string targetNodeId;
    std::string summary;
    double threshold = 0.0;
    double durationSeconds = 0.0;
    bool startsActive = false;
    std::vector<ObjectiveReward> rewards;
    std::vector<std::string> nextObjectives;
};

struct ScenarioPhase {
    std::string name;
    std::string eventMessage;
    double startTimeSeconds = 0.0;
    NumericRange startTimeSecondsRange{0.0, 0.0};
    double durationSeconds = 30.0;
    NumericRange durationSecondsRange{30.0, 30.0};
    GameplayDuration transitionDuration;
    double trafficMultiplier = 1.0;
    NumericRange trafficMultiplierRange{1.0, 1.0};
    std::optional<BurstScenario> burstOverride;
    std::vector<MechanicType> unlockMechanics;
};

struct LinkScenario {
    int sourceNode = 0;
    int targetNode = 0;
    double baseLatencySeconds = 0.6;
    double bandwidthPerSecond = 100.0;
};

struct ScenarioDefinition {
    std::string id;
    std::string displayName;
    std::vector<std::string> tags;
    std::string name;
    std::string description;
    ScenarioArchetype archetype = ScenarioArchetype::LocalStartup;
    ProgressionTier minimumTier = ProgressionTier::Foundations;
    std::string topologyTemplateId;
    std::vector<EducationalFocus> educationalFocus;
    std::string initialTopologyTemplate = "default-regional-api";
    std::vector<PressureCategory> guaranteedPressures;
    std::vector<ScenarioModifierDefinition> optionalModifiers;
    std::vector<MechanicType> allowedMechanics;
    std::vector<MechanicType> startingInterventions;
    std::vector<MechanicType> unlockableInterventions;
    std::vector<MechanicType> disabledInterventions;
    std::vector<MechanicType> recommendedMechanics;
    std::vector<std::string> unlocksScenarios;
    std::vector<std::string> requiredCompletedScenarios;
    std::vector<std::string> requiredConceptTags;
    EngineeringCapacity engineeringCapacity;
    bool sandboxLab = false;
    std::vector<NodeScenario> nodes;
    std::vector<LinkScenario> links;
    TrafficProfile trafficProfile;
    RequestTypeScenario requestTypes;
    CacheScenario cache;
    RetryScenario retries;
    BurstScenario bursts;
    std::vector<ScenarioObjective> objectives;
    std::vector<ScenarioObjective> failureConditions;
    std::vector<ScenarioPhase> phases;
    std::vector<EventDefinition> events;
    std::vector<EventDefinition> sandboxEvents;
    GameplayDuration turnDuration;
    double requestTimeoutSeconds = 5.5;
};

struct ScenarioRun {
    std::uint32_t seed = 0;
    std::vector<ScenarioModifierDefinition> selectedModifiers;
    ScenarioRunState state = ScenarioRunState::Running;
    double elapsedSeconds = 0.0;
    double calendarElapsedDays = 0.0;
    double objectiveProgress = 0.0;
    int currentPhaseIndex = -1;
    ScenarioDefinition activeDefinition;
    std::vector<std::string> activeObjectiveIds;
    std::vector<std::string> completedObjectiveIds;
    std::vector<MechanicType> unlockedInterventions;
    std::vector<std::string> unlockedScenarioIds;
    std::vector<std::string> unlockedMetrics;
    std::vector<std::string> unlockedOverlays;
    std::vector<std::string> feedbackMessages;
};

struct ProgressionState {
    std::set<std::string> completedScenarios;
    std::set<std::string> unlockedScenarios;
    std::set<std::string> unlockedConcepts;
    std::set<std::string> unlockedMetrics;
    std::set<std::string> unlockedOverlays;
};

struct SandboxControls {
    double trafficMultiplier = 1.0;
    double latencyMultiplier = 1.0;
    double databaseCapacityMultiplier = 1.0;
    bool queueBuildup = false;
    std::uint32_t seed = 1;
};

class ProgressionRegistry {
public:
    [[nodiscard]] static const std::vector<ProgressionTierDefinition>& definitions();
    [[nodiscard]] static const ProgressionTierDefinition& definition(ProgressionTier tier);
};

class Scenario {
public:
    static ScenarioDefinition createDefault();
};

class ScenarioRegistry {
public:
    static std::vector<ScenarioDefinition> createAll();
    static ScenarioDefinition singleServiceOverload();
    static ScenarioDefinition databaseBottleneck();
    static ScenarioDefinition burstTraffic();
};

const char* progressionTierName(ProgressionTier tier);
const char* scenarioArchetypeName(ScenarioArchetype archetype);
const char* engineeringDomainName(EngineeringDomain domain);
const char* gameplayDurationUnitName(GameplayDurationUnit unit);
double gameplayDurationCalendarDays(const GameplayDuration& duration);
