#include "gameplay/Scenario.hpp"

ScenarioDefinition Scenario::createDefault()
{
    ScenarioDefinition scenario;
    scenario.name = "Single service pressure test";

    scenario.nodes = {
        {.name = "Web clients", .type = NodeType::ClientCluster, .position = {-420.0f, -120.0f}, .requestRatePerSecond = 4.0},
        {.name = "Mobile clients", .type = NodeType::ClientCluster, .position = {-420.0f, 150.0f}, .requestRatePerSecond = 3.0},
        {.name = "API service", .type = NodeType::Service, .position = {80.0f, 0.0f}, .processingCapacityPerSecond = 6.0, .timeoutSeconds = 5.0},
        {.name = "Database", .type = NodeType::Database, .position = {430.0f, 0.0f}, .processingCapacityPerSecond = 12.0, .timeoutSeconds = 8.0},
    };

    scenario.links = {
        {.sourceNode = 0, .targetNode = 2, .baseLatencySeconds = 0.85, .bandwidthPerSecond = 40.0},
        {.sourceNode = 1, .targetNode = 2, .baseLatencySeconds = 0.95, .bandwidthPerSecond = 40.0},
        {.sourceNode = 2, .targetNode = 3, .baseLatencySeconds = 0.35, .bandwidthPerSecond = 80.0},
    };

    return scenario;
}
