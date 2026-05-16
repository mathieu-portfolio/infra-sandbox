#include "gameplay/Scenario.hpp"

ScenarioDefinition Scenario::createDefault()
{
    ScenarioDefinition scenario;
    scenario.name = "API and database pressure test";

    scenario.nodes = {
        {.name = "Web clients", .type = NodeType::ClientCluster, .position = {-420.0f, -120.0f}, .requestRatePerSecond = 4.0},
        {.name = "Mobile clients", .type = NodeType::ClientCluster, .position = {-420.0f, 150.0f}, .requestRatePerSecond = 3.0},
        {.name = "Cache", .type = NodeType::Cache, .position = {80.0f, -165.0f}},
        {.name = "API service", .type = NodeType::ApiService, .position = {80.0f, 0.0f}, .processingCapacityPerSecond = 7.0, .timeoutSeconds = 5.5},
        {.name = "Database", .type = NodeType::Database, .position = {430.0f, 0.0f}, .processingCapacityPerSecond = 4.0, .timeoutSeconds = 5.5},
    };

    scenario.links = {
        {.sourceNode = 0, .targetNode = 3, .baseLatencySeconds = 0.85, .bandwidthPerSecond = 40.0},
        {.sourceNode = 1, .targetNode = 3, .baseLatencySeconds = 0.95, .bandwidthPerSecond = 40.0},
        {.sourceNode = 3, .targetNode = 4, .baseLatencySeconds = 0.35, .bandwidthPerSecond = 80.0},
        {.sourceNode = 4, .targetNode = 3, .baseLatencySeconds = 0.35, .bandwidthPerSecond = 80.0},
    };

    scenario.requestTypes.lightweightShare = 0.64;
    scenario.requestTypes.databaseHeavyCacheableShare = 0.7;
    scenario.cache.enabled = false;
    scenario.cache.maxEntries = 8;
    scenario.cache.ttlSeconds = 12.0;
    scenario.retries.enabled = true;
    scenario.retries.maxRetries = 1;
    scenario.retries.retryDelaySeconds = 0.85;
    scenario.bursts.enabled = false;

    return scenario;
}
