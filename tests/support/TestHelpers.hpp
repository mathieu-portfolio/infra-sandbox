#pragma once

#include "content/loading/ContentPackManager.hpp"
#include "core/simulation/Mechanics.hpp"
#include "gameplay/Scenario.hpp"
#include "simulation/core/Simulation.hpp"

#include <string>

namespace test_support {

inline void runFor(Simulation& simulation, double seconds)
{
    constexpr double fixedStep = 1.0 / 60.0;
    const int steps = static_cast<int>(seconds / fixedStep);
    for (int i = 0; i < steps; ++i) {
        simulation.update(fixedStep);
    }
}

inline EngineeringCapacity capacityBonus(int frontend, int backend, int infrastructure, int data, int total)
{
    EngineeringCapacity bonus;
    bonus.frontend = frontend;
    bonus.backend = backend;
    bonus.infrastructure = infrastructure;
    bonus.data = data;
    bonus.total = total;
    return bonus;
}

inline std::string joinedErrors(const content::ContentLoadResult& result)
{
    std::string errors;
    for (const auto& error : result.errors) {
        errors += error + "\n";
    }
    return errors;
}

inline ScenarioDefinition saturatedScenario()
{
    ScenarioDefinition scenario;
    scenario.name = "Saturated API test";
    scenario.nodes = {
        {.name = "Clients", .type = NodeType::ClientCluster, .position = {-100.0f, 0.0f}, .requestRatePerSecond = 12.0},
        {.name = "API", .type = NodeType::ApiService, .position = {100.0f, 0.0f}, .processingCapacityPerSecond = 1.0, .timeoutSeconds = 1.5},
    };
    scenario.links = {
        {.sourceNode = 0, .targetNode = 1, .baseLatencySeconds = 0.1, .bandwidthPerSecond = 100.0},
    };
    scenario.requestTimeoutSeconds = 1.5;
    return scenario;
}

inline ScenarioDefinition databasePressureScenario()
{
    ScenarioDefinition scenario;
    scenario.name = "Database pressure test";
    scenario.nodes = {
        {.name = "Clients", .type = NodeType::ClientCluster, .position = {-100.0f, 0.0f}, .requestRatePerSecond = 8.0},
        {.name = "API", .type = NodeType::ApiService, .position = {100.0f, 0.0f}, .processingCapacityPerSecond = 20.0, .timeoutSeconds = 4.0},
        {.name = "DB", .type = NodeType::Database, .position = {300.0f, 0.0f}, .processingCapacityPerSecond = 1.0, .timeoutSeconds = 4.0},
    };
    scenario.links = {
        {.sourceNode = 0, .targetNode = 1, .baseLatencySeconds = 0.05, .bandwidthPerSecond = 100.0},
        {.sourceNode = 1, .targetNode = 2, .baseLatencySeconds = 0.05, .bandwidthPerSecond = 100.0},
        {.sourceNode = 2, .targetNode = 1, .baseLatencySeconds = 0.05, .bandwidthPerSecond = 100.0},
    };
    scenario.requestTypes.lightweightShare = 0.0;
    scenario.requestTypes.databaseHeavyCacheableShare = 1.0;
    scenario.cache.enabled = false;
    scenario.retries.enabled = true;
    scenario.retries.maxRetries = 1;
    scenario.requestTimeoutSeconds = 2.0;
    return scenario;
}

inline ScenarioDefinition adaptiveTrafficScenario()
{
    ScenarioDefinition scenario;
    scenario.name = "Adaptive traffic test";
    scenario.nodes = {
        {.name = "Clients", .type = NodeType::ClientCluster, .position = {-100.0f, 0.0f}, .requestRatePerSecond = 4.0},
        {.name = "Slow API", .type = NodeType::ApiService, .position = {100.0f, -80.0f}, .processingCapacityPerSecond = 20.0, .timeoutSeconds = 3.0},
        {.name = "Fast API", .type = NodeType::ApiService, .position = {100.0f, 80.0f}, .processingCapacityPerSecond = 20.0, .timeoutSeconds = 3.0},
    };
    scenario.links = {
        {.sourceNode = 0, .targetNode = 1, .baseLatencySeconds = 2.0, .bandwidthPerSecond = 100.0},
        {.sourceNode = 0, .targetNode = 2, .baseLatencySeconds = 0.05, .bandwidthPerSecond = 100.0},
    };
    scenario.trafficProfile.evolution.enabled = true;
    scenario.trafficProfile.evolution.rerouteLatencySensitivity = 1.0;
    scenario.requestTimeoutSeconds = 3.0;
    return scenario;
}

} // namespace test_support
