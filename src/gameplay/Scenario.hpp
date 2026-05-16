#pragma once

#include "simulation/Mechanics.hpp"
#include "simulation/InfrastructureGraph.hpp"

#include <optional>
#include <string>
#include <vector>

struct NodeScenario {
    std::string name;
    NodeType type = NodeType::ApiService;
    Vec2 position{};
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
    double periodSeconds = 12.0;
    double durationSeconds = 3.0;
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
    TrafficProfileType type = TrafficProfileType::Constant;
    double baseMultiplier = 1.0;
    double growthPerSecond = 0.0;
};

enum class ScenarioObjectiveType {
    MaxLatency,
    MaxErrorRate,
    MinThroughput,
    SurviveDuration,
    StabilizeQueues
};

struct ScenarioObjective {
    ScenarioObjectiveType type = ScenarioObjectiveType::SurviveDuration;
    std::string summary;
    double threshold = 0.0;
    double durationSeconds = 0.0;
};

struct ScenarioPhase {
    std::string name;
    std::string eventMessage;
    double startTimeSeconds = 0.0;
    double durationSeconds = 30.0;
    double trafficMultiplier = 1.0;
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
    std::string name;
    std::string description;
    std::vector<EducationalFocus> educationalFocus;
    std::vector<MechanicType> allowedMechanics;
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
    double requestTimeoutSeconds = 5.5;
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
