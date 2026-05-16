#include "gameplay/Scenario.hpp"

namespace {
ScenarioDefinition baseTopology()
{
    ScenarioDefinition scenario;
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
    scenario.requestTimeoutSeconds = 5.5;
    return scenario;
}
}

ScenarioDefinition Scenario::createDefault()
{
    return ScenarioRegistry::singleServiceOverload();
}

std::vector<ScenarioDefinition> ScenarioRegistry::createAll()
{
    return {
        singleServiceOverload(),
        databaseBottleneck(),
        burstTraffic(),
    };
}

ScenarioDefinition ScenarioRegistry::singleServiceOverload()
{
    auto scenario = baseTopology();
    scenario.name = "Single Service Overload";
    scenario.description = "Traffic gradually outgrows API processing capacity.";
    scenario.educationalFocus = {EducationalFocus::Scaling, EducationalFocus::Queues, EducationalFocus::Latency};
    scenario.allowedMechanics = {MechanicType::ScaleUp, MechanicType::ThrottleTraffic};
    scenario.trafficProfile = {.type = TrafficProfileType::GradualGrowth, .baseMultiplier = 0.75, .growthPerSecond = 0.012};
    scenario.nodes[3].processingCapacityPerSecond = 5.0;
    scenario.requestTypes.lightweightShare = 0.82;
    scenario.phases = {
        {.name = "Stable warm-up", .eventMessage = "Demand is within API capacity.", .startTimeSeconds = 0.0, .durationSeconds = 25.0, .trafficMultiplier = 0.75},
        {.name = "Demand exceeds API", .eventMessage = "Traffic now exceeds single-service throughput.", .startTimeSeconds = 25.0, .durationSeconds = 45.0, .trafficMultiplier = 1.35},
        {.name = "Sustained pressure", .eventMessage = "Stabilize queues while demand remains high.", .startTimeSeconds = 70.0, .durationSeconds = 90.0, .trafficMultiplier = 1.55},
    };
    scenario.objectives = {
        {.type = ScenarioObjectiveType::SurviveDuration, .summary = "Keep the service operating for 120s.", .durationSeconds = 120.0},
        {.type = ScenarioObjectiveType::StabilizeQueues, .summary = "Reduce API queue pressure after overload appears.", .threshold = 8.0},
    };
    scenario.failureConditions = {
        {.type = ScenarioObjectiveType::MaxLatency, .summary = "Latency above 6s indicates instability.", .threshold = 6.0},
    };
    return scenario;
}

ScenarioDefinition ScenarioRegistry::databaseBottleneck()
{
    auto scenario = baseTopology();
    scenario.name = "Database Bottleneck";
    scenario.description = "API scaling cannot remove downstream database pressure.";
    scenario.educationalFocus = {EducationalFocus::Persistence, EducationalFocus::Caching, EducationalFocus::Reliability};
    scenario.allowedMechanics = {MechanicType::ScaleUp, MechanicType::EnableCache, MechanicType::ClearCache, MechanicType::ToggleRetries, MechanicType::ThrottleTraffic};
    scenario.trafficProfile = {.type = TrafficProfileType::GradualGrowth, .baseMultiplier = 0.85, .growthPerSecond = 0.006};
    scenario.nodes[3].processingCapacityPerSecond = 10.0;
    scenario.nodes[4].processingCapacityPerSecond = 2.4;
    scenario.requestTypes.lightweightShare = 0.28;
    scenario.requestTypes.databaseHeavyCacheableShare = 0.8;
    scenario.cache.maxEntries = 10;
    scenario.phases = {
        {.name = "Healthy reads", .eventMessage = "Database work is present but manageable.", .startTimeSeconds = 0.0, .durationSeconds = 25.0, .trafficMultiplier = 0.8},
        {.name = "DB queue forms", .eventMessage = "Downstream persistence becomes the constraint.", .startTimeSeconds = 25.0, .durationSeconds = 55.0, .trafficMultiplier = 1.25},
        {.name = "Cache opportunity", .eventMessage = "Repeated reads can be absorbed by cache.", .startTimeSeconds = 80.0, .durationSeconds = 90.0, .trafficMultiplier = 1.35},
    };
    scenario.objectives = {
        {.type = ScenarioObjectiveType::SurviveDuration, .summary = "Manage DB pressure for 120s.", .durationSeconds = 120.0},
    };
    scenario.failureConditions = {
        {.type = ScenarioObjectiveType::MaxErrorRate, .summary = "Timeout rate above 3/s is unstable.", .threshold = 3.0},
    };
    return scenario;
}

ScenarioDefinition ScenarioRegistry::burstTraffic()
{
    auto scenario = baseTopology();
    scenario.name = "Burst Traffic";
    scenario.description = "Short traffic spikes expose queues, timeouts, and retry amplification.";
    scenario.educationalFocus = {EducationalFocus::Queues, EducationalFocus::Reliability, EducationalFocus::Latency};
    scenario.allowedMechanics = {MechanicType::ScaleUp, MechanicType::ToggleRetries, MechanicType::ThrottleTraffic};
    scenario.trafficProfile = {.type = TrafficProfileType::Bursty, .baseMultiplier = 0.9, .growthPerSecond = 0.0};
    scenario.nodes[3].processingCapacityPerSecond = 6.0;
    scenario.requestTypes.lightweightShare = 0.7;
    scenario.bursts = {.enabled = true, .multiplier = 2.4, .periodSeconds = 16.0, .durationSeconds = 4.0};
    scenario.phases = {
        {.name = "Baseline", .eventMessage = "Traffic is calm between waves.", .startTimeSeconds = 0.0, .durationSeconds = 20.0, .trafficMultiplier = 0.85},
        {.name = "Burst waves", .eventMessage = "Periodic spikes stress queues and retries.", .startTimeSeconds = 20.0, .durationSeconds = 70.0, .trafficMultiplier = 1.0, .burstOverride = BurstScenario{true, 2.6, 14.0, 4.0}},
        {.name = "Recovery window", .eventMessage = "Watch whether queues drain after each wave.", .startTimeSeconds = 90.0, .durationSeconds = 60.0, .trafficMultiplier = 1.1, .burstOverride = BurstScenario{true, 2.0, 18.0, 3.0}},
    };
    scenario.objectives = {
        {.type = ScenarioObjectiveType::SurviveDuration, .summary = "Absorb burst traffic for 120s.", .durationSeconds = 120.0},
    };
    scenario.failureConditions = {
        {.type = ScenarioObjectiveType::MaxErrorRate, .summary = "Retry storms above 4 timeouts/s are unstable.", .threshold = 4.0},
    };
    return scenario;
}
