#pragma once

#include "simulation/InfrastructureGraph.hpp"

#include <string>
#include <vector>

struct NodeScenario {
    std::string name;
    NodeType type = NodeType::Service;
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

struct LinkScenario {
    int sourceNode = 0;
    int targetNode = 0;
    double baseLatencySeconds = 0.6;
    double bandwidthPerSecond = 100.0;
};

struct ScenarioDefinition {
    std::string name;
    std::vector<NodeScenario> nodes;
    std::vector<LinkScenario> links;
    RequestTypeScenario requestTypes;
    CacheScenario cache;
    RetryScenario retries;
    BurstScenario bursts;
    double requestTimeoutSeconds = 5.5;
};

class Scenario {
public:
    static ScenarioDefinition createDefault();
};
