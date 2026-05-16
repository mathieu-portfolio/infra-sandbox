#include "gameplay/Scenario.hpp"

#include <algorithm>

namespace {
EventDefinition trafficSpike(double atSeconds);
EventDefinition databaseSlowdown();
EventDefinition retryStorm();

ScenarioModifierDefinition mobileRefreshWave()
{
    return {
        .type = ScenarioModifierType::MobileRefreshWave,
        .name = "Mobile refresh wave",
        .description = "A mild burst of client refreshes shifts timing without changing the lesson.",
        .selectionWeight = 0.35,
        .burstOverride = BurstScenario{true, 1.45, 22.0, 3.0},
    };
}

ScenarioModifierDefinition readHeavyBehavior()
{
    return {
        .type = ScenarioModifierType::ReadHeavyBehavior,
        .name = "Read-heavy behavior",
        .description = "Repeated read patterns make cache effects easier to observe.",
        .selectionWeight = 0.45,
        .databaseHeavyShare = 0.72,
    };
}

ScenarioModifierDefinition aggressiveRetries()
{
    return {
        .type = ScenarioModifierType::AggressiveRetries,
        .name = "Aggressive retries",
        .description = "Retry timing varies the feedback pressure.",
        .selectionWeight = 0.3,
        .events = {retryStorm()},
    };
}

EventDefinition trafficSpike(double atSeconds)
{
    return {
        .name = "TrafficSpike",
        .description = "A short-lived traffic surge increases request pressure.",
        .category = EventCategory::TrafficEvent,
        .trigger = {.type = EventTriggerType::TimeBased, .timeSeconds = atSeconds},
        .effect = {.type = EventEffectType::TrafficSpike, .trafficMultiplier = 1.35, .burstMultiplier = 1.25},
        .durationSeconds = 18.0,
    };
}

EventDefinition viralGrowth(double atSeconds)
{
    return {
        .name = "ViralGrowth",
        .description = "Demand increases and remains elevated for a while.",
        .category = EventCategory::DemandEvent,
        .trigger = {.type = EventTriggerType::TimeBased, .timeSeconds = atSeconds},
        .effect = {.type = EventEffectType::ViralGrowth, .trafficMultiplier = 1.45},
        .durationSeconds = 42.0,
    };
}

EventDefinition databaseSlowdown()
{
    return {
        .name = "DatabaseSlowdown",
        .description = "Storage latency reduces database effective capacity.",
        .category = EventCategory::InfrastructureEvent,
        .trigger = {.type = EventTriggerType::MetricThreshold, .metric = EventMetric::DatabaseQueue, .threshold = 5.0, .delaySeconds = 3.0},
        .effect = {.type = EventEffectType::DatabaseSlowdown, .databaseCapacityMultiplier = 0.62},
        .durationSeconds = 28.0,
    };
}

EventDefinition retryStorm()
{
    return {
        .name = "RetryStorm",
        .description = "Timeouts cause retries to amplify load after a delay.",
        .category = EventCategory::ReliabilityEvent,
        .trigger = {.type = EventTriggerType::MetricThreshold, .metric = EventMetric::RetryRate, .threshold = 0.5, .delaySeconds = 2.0},
        .effect = {.type = EventEffectType::RetryStorm, .trafficMultiplier = 1.18, .retryDelayMultiplier = 0.75},
        .durationSeconds = 22.0,
    };
}

EventDefinition cacheWarmup()
{
    return {
        .name = "CacheWarmup",
        .description = "Cache effectiveness becomes visible after repeated reads.",
        .category = EventCategory::RecoveryEvent,
        .trigger = {.type = EventTriggerType::MetricThreshold, .metric = EventMetric::CacheHitRate, .threshold = 0.35},
        .effect = {.type = EventEffectType::CacheWarmup, .databaseCapacityMultiplier = 1.08},
        .durationSeconds = 24.0,
    };
}

EventDefinition partialRecovery(double atSeconds)
{
    return {
        .name = "PartialRecovery",
        .description = "Pressure eases temporarily as systems recover.",
        .category = EventCategory::RecoveryEvent,
        .trigger = {.type = EventTriggerType::TimeBased, .timeSeconds = atSeconds},
        .effect = {.type = EventEffectType::PartialRecovery, .trafficMultiplier = 0.88, .databaseCapacityMultiplier = 1.15},
        .durationSeconds = 20.0,
    };
}

ScenarioModifierDefinition regionalTrafficSpike(double atSeconds)
{
    return {
        .type = ScenarioModifierType::RegionalTrafficSpike,
        .name = "Regional traffic spike",
        .description = "A traffic event varies the pressure cadence.",
        .selectionWeight = 0.4,
        .events = {trafficSpike(atSeconds)},
    };
}

ScenarioModifierDefinition slowDatabaseWindow()
{
    return {
        .type = ScenarioModifierType::SlowDatabaseWindow,
        .name = "Slow database window",
        .description = "A temporary storage slowdown varies persistence pressure.",
        .selectionWeight = 0.35,
        .events = {databaseSlowdown()},
    };
}

ScenarioDefinition baseTopology()
{
    ScenarioDefinition scenario;
    scenario.nodes = {
        {
            .name = "NA users",
            .type = NodeType::ClientCluster,
            .geoLocation = GeoLocation{37.77, -122.42, "NorthAmerica"},
            .networkIdentity = {"clients.na.infrastructure.local", "10.10.0.10", "https://clients.na.example.net"},
            .requestRatePerSecond = 4.0,
        },
        {
            .name = "EU users",
            .type = NodeType::ClientCluster,
            .geoLocation = GeoLocation{52.52, 13.40, "Europe"},
            .networkIdentity = {"clients.eu.infrastructure.local", "10.20.0.10", "https://clients.eu.example.net"},
            .requestRatePerSecond = 3.0,
        },
        {
            .name = "NA cache",
            .type = NodeType::Cache,
            .geoLocation = GeoLocation{39.10, -94.58, "NorthAmerica"},
            .networkIdentity = {"cache.na-edge.infrastructure.local", "10.10.1.20", "https://cache.na-edge.example.net"},
        },
        {
            .name = "EU API service",
            .type = NodeType::ApiService,
            .geoLocation = GeoLocation{50.11, 8.68, "Europe"},
            .networkIdentity = {"api.eu-west.infrastructure.local", "10.20.0.20", "https://api.eu-west.example.net"},
            .processingCapacityPerSecond = 7.0,
            .timeoutSeconds = 5.5,
        },
        {
            .name = "EU database",
            .type = NodeType::Database,
            .geoLocation = GeoLocation{50.12, 8.67, "Europe"},
            .networkIdentity = {"db.eu-west.infrastructure.local", "10.20.1.30", "https://db.eu-west.example.net"},
            .processingCapacityPerSecond = 4.0,
            .timeoutSeconds = 5.5,
        },
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

const std::vector<ProgressionTierDefinition>& ProgressionRegistry::definitions()
{
    static const std::vector<ProgressionTierDefinition> definitions{
        {
            .tier = ProgressionTier::Foundations,
            .name = "Foundations",
            .visibleMetrics = {"Input rate", "Processed req/s", "Queue depth", "Latency"},
            .availableMechanics = {MechanicType::ScaleUp, MechanicType::ThrottleTraffic},
            .allowedPressures = {PressureCategory::TrafficPressure, PressureCategory::QueuePressure, PressureCategory::LatencyPressure},
            .allowedNodeTypes = {NodeType::ClientCluster, NodeType::ApiService},
        },
        {
            .tier = ProgressionTier::LocalScale,
            .name = "Local Scale",
            .visibleMetrics = {"Input rate", "Processed req/s", "API utilization", "Queue depth", "Latency"},
            .availableMechanics = {MechanicType::ScaleUp, MechanicType::ScaleOut, MechanicType::ThrottleTraffic},
            .allowedPressures = {PressureCategory::TrafficPressure, PressureCategory::QueuePressure, PressureCategory::ComputePressure, PressureCategory::LatencyPressure},
            .allowedNodeTypes = {NodeType::ClientCluster, NodeType::ApiService, NodeType::LoadBalancer},
        },
        {
            .tier = ProgressionTier::StateAndCache,
            .name = "State and Cache",
            .visibleMetrics = {"DB queue", "Cache hit rate", "Timeout rate", "Latency"},
            .availableMechanics = {MechanicType::ScaleUp, MechanicType::EnableCache, MechanicType::ClearCache, MechanicType::AddCache, MechanicType::AddReadReplica, MechanicType::ToggleRetries, MechanicType::ThrottleTraffic},
            .allowedPressures = {PressureCategory::QueuePressure, PressureCategory::PersistencePressure, PressureCategory::LatencyPressure},
            .allowedNodeTypes = {NodeType::ClientCluster, NodeType::ApiService, NodeType::Database, NodeType::Cache},
        },
        {
            .tier = ProgressionTier::FailureFeedback,
            .name = "Failure Feedback",
            .visibleMetrics = {"Timeout rate", "Retry rate", "Queue depth", "Latency"},
            .availableMechanics = {MechanicType::ScaleUp, MechanicType::AddQueue, MechanicType::ToggleRetries, MechanicType::ThrottleTraffic},
            .allowedPressures = {PressureCategory::RetryPressure, PressureCategory::FailurePressure, PressureCategory::QueuePressure, PressureCategory::LatencyPressure},
            .allowedNodeTypes = {NodeType::ClientCluster, NodeType::ApiService, NodeType::Database, NodeType::RetryController},
        },
        {
            .tier = ProgressionTier::GeographicScale,
            .name = "Geographic Scale",
            .visibleMetrics = {"Latency", "Regional traffic", "Queue depth", "Error rate"},
            .availableMechanics = {MechanicType::ScaleUp, MechanicType::EnableCache, MechanicType::AddRegionalCache, MechanicType::ThrottleTraffic},
            .allowedPressures = {PressureCategory::TrafficPressure, PressureCategory::LatencyPressure, PressureCategory::PersistencePressure},
            .allowedNodeTypes = {NodeType::ClientCluster, NodeType::ApiService, NodeType::Database, NodeType::Cache, NodeType::CDNEdge},
        },
        {
            .tier = ProgressionTier::DistributedSystems,
            .name = "Distributed Systems",
            .visibleMetrics = {"Latency", "Retry rate", "DB queue", "Cache hit rate"},
            .availableMechanics = {MechanicType::ScaleUp, MechanicType::EnableCache, MechanicType::AddCache, MechanicType::AddRegionalCache, MechanicType::AddReadReplica, MechanicType::AddLoadBalancer, MechanicType::ThrottleTraffic},
            .allowedPressures = {PressureCategory::TrafficPressure, PressureCategory::QueuePressure, PressureCategory::PersistencePressure, PressureCategory::RetryPressure},
            .allowedNodeTypes = {NodeType::ClientCluster, NodeType::ApiService, NodeType::Database, NodeType::Cache, NodeType::ReadReplica, NodeType::LoadBalancer},
        },
        {
            .tier = ProgressionTier::Complexity,
            .name = "Complexity",
            .visibleMetrics = {"All core metrics"},
            .availableMechanics = {MechanicType::ScaleUp, MechanicType::ScaleOut, MechanicType::EnableCache, MechanicType::ClearCache, MechanicType::ToggleRetries, MechanicType::AddCache, MechanicType::AddQueue, MechanicType::AddReadReplica, MechanicType::AddRegionalCache, MechanicType::AddLoadBalancer, MechanicType::ThrottleTraffic, MechanicType::EnableTracing},
            .allowedPressures = {PressureCategory::TrafficPressure, PressureCategory::QueuePressure, PressureCategory::ComputePressure, PressureCategory::PersistencePressure, PressureCategory::RetryPressure, PressureCategory::LatencyPressure, PressureCategory::FailurePressure},
            .allowedNodeTypes = {},
        },
    };
    return definitions;
}

const ProgressionTierDefinition& ProgressionRegistry::definition(ProgressionTier tier)
{
    const auto& entries = definitions();
    const auto it = std::find_if(entries.begin(), entries.end(), [tier](const ProgressionTierDefinition& definition) {
        return definition.tier == tier;
    });
    return it != entries.end() ? *it : entries.front();
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
    scenario.archetype = ScenarioArchetype::LocalStartup;
    scenario.minimumTier = ProgressionTier::LocalScale;
    scenario.educationalFocus = {EducationalFocus::Scaling, EducationalFocus::Queues, EducationalFocus::Latency};
    scenario.guaranteedPressures = {PressureCategory::TrafficPressure, PressureCategory::QueuePressure, PressureCategory::ComputePressure};
    scenario.allowedMechanics = {MechanicType::ScaleUp, MechanicType::ThrottleTraffic};
    scenario.recommendedMechanics = {MechanicType::ScaleUp};
    scenario.trafficProfile = {.name = "Gradual growth", .type = TrafficProfileType::GradualGrowth, .baseMultiplier = 0.75, .growthPerSecond = 0.012};
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
    scenario.events = {
        trafficSpike(40.0),
        viralGrowth(76.0),
        partialRecovery(116.0),
    };
    scenario.optionalModifiers = {
        mobileRefreshWave(),
        regionalTrafficSpike(58.0),
    };
    return scenario;
}

ScenarioDefinition ScenarioRegistry::databaseBottleneck()
{
    auto scenario = baseTopology();
    scenario.name = "Database Bottleneck";
    scenario.description = "API scaling cannot remove downstream database pressure.";
    scenario.archetype = ScenarioArchetype::DatabasePressure;
    scenario.minimumTier = ProgressionTier::StateAndCache;
    scenario.educationalFocus = {EducationalFocus::Persistence, EducationalFocus::Caching, EducationalFocus::Reliability};
    scenario.guaranteedPressures = {PressureCategory::PersistencePressure, PressureCategory::QueuePressure, PressureCategory::LatencyPressure};
    scenario.allowedMechanics = {MechanicType::ScaleUp, MechanicType::EnableCache, MechanicType::ClearCache, MechanicType::AddCache, MechanicType::AddReadReplica, MechanicType::ToggleRetries, MechanicType::ThrottleTraffic};
    scenario.recommendedMechanics = {MechanicType::EnableCache, MechanicType::ClearCache};
    scenario.trafficProfile = {.name = "Read pressure growth", .type = TrafficProfileType::GradualGrowth, .baseMultiplier = 0.85, .growthPerSecond = 0.006};
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
    scenario.events = {
        databaseSlowdown(),
        retryStorm(),
        cacheWarmup(),
        partialRecovery(120.0),
    };
    scenario.optionalModifiers = {
        readHeavyBehavior(),
        slowDatabaseWindow(),
        aggressiveRetries(),
    };
    return scenario;
}

ScenarioDefinition ScenarioRegistry::burstTraffic()
{
    auto scenario = baseTopology();
    scenario.name = "Burst Traffic";
    scenario.description = "Short traffic spikes expose queues, timeouts, and retry amplification.";
    scenario.archetype = ScenarioArchetype::BurstTraffic;
    scenario.minimumTier = ProgressionTier::FailureFeedback;
    scenario.educationalFocus = {EducationalFocus::Queues, EducationalFocus::Reliability, EducationalFocus::Latency};
    scenario.guaranteedPressures = {PressureCategory::TrafficPressure, PressureCategory::QueuePressure, PressureCategory::RetryPressure};
    scenario.allowedMechanics = {MechanicType::ScaleUp, MechanicType::AddQueue, MechanicType::ToggleRetries, MechanicType::ThrottleTraffic};
    scenario.recommendedMechanics = {MechanicType::ToggleRetries, MechanicType::ThrottleTraffic};
    scenario.trafficProfile = {.name = "Bursty", .type = TrafficProfileType::Bursty, .baseMultiplier = 0.9, .growthPerSecond = 0.0};
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
    scenario.events = {
        trafficSpike(26.0),
        retryStorm(),
        trafficSpike(64.0),
        partialRecovery(106.0),
    };
    scenario.optionalModifiers = {
        mobileRefreshWave(),
        regionalTrafficSpike(44.0),
        aggressiveRetries(),
    };
    return scenario;
}

const char* progressionTierName(ProgressionTier tier)
{
    switch (tier) {
    case ProgressionTier::Foundations:
        return "Foundations";
    case ProgressionTier::LocalScale:
        return "Local Scale";
    case ProgressionTier::StateAndCache:
        return "State and Cache";
    case ProgressionTier::FailureFeedback:
        return "Failure Feedback";
    case ProgressionTier::GeographicScale:
        return "Geographic Scale";
    case ProgressionTier::DistributedSystems:
        return "Distributed Systems";
    case ProgressionTier::Complexity:
        return "Complexity";
    }
    return "Unknown";
}

const char* scenarioArchetypeName(ScenarioArchetype archetype)
{
    switch (archetype) {
    case ScenarioArchetype::FirstRequest:
        return "First Request";
    case ScenarioArchetype::LocalStartup:
        return "Local Startup";
    case ScenarioArchetype::DatabasePressure:
        return "Database Pressure";
    case ScenarioArchetype::BurstTraffic:
        return "Burst Traffic";
    case ScenarioArchetype::TransatlanticLatency:
        return "Transatlantic Latency";
    case ScenarioArchetype::GlobalReadPlatform:
        return "Global Read Platform";
    }
    return "Unknown";
}
