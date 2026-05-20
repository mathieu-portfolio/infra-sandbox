#pragma once

#include "gameplay/scenario/data/ScenarioEnums.hpp"
#include "simulation/core/Mechanics.hpp"

#include <string>
#include <vector>

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

struct TrafficEvolutionScenario {
    bool enabled = false;
    double pressureSensitivity = 0.0;
    double churnSensitivity = 0.0;
    double migrationSensitivity = 0.0;
    double reroutePressureSensitivity = 0.0;
    double rerouteLatencySensitivity = 0.0;
    double dynamicRetrySensitivity = 0.0;
    double burstAmplification = 0.0;
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
    TrafficEvolutionScenario evolution;
};
