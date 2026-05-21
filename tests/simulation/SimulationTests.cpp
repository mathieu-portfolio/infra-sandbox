#include "gameplay/Scenario.hpp"
#include "gameplay/scenario/ScenarioManager.hpp"
#include "core/simulation/Mechanics.hpp"
#include "simulation/core/Simulation.hpp"
#include "simulation/metrics/CrossDomainInteraction.hpp"
#include "core/topology/Geography.hpp"
#include "core/topology/NodeDefinition.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

namespace {
void runFor(Simulation& simulation, double seconds)
{
    constexpr double fixedStep = 1.0 / 60.0;
    const int steps = static_cast<int>(seconds / fixedStep);
    for (int i = 0; i < steps; ++i) {
        simulation.update(fixedStep);
    }
}

EngineeringCapacity capacityBonus(int frontend, int backend, int infrastructure, int data, int total)
{
    EngineeringCapacity bonus;
    bonus.frontend = frontend;
    bonus.backend = backend;
    bonus.infrastructure = infrastructure;
    bonus.data = data;
    bonus.total = total;
    return bonus;
}

TEST(NodeRegistryTests, RegistersFutureNodeSkeletons)
{
    EXPECT_EQ(NodeRegistry::definitions().size(), 50U);
    EXPECT_EQ(NodeRegistry::categoryOf(NodeType::ClientCluster), NodeCategory::Demand);
    EXPECT_EQ(NodeRegistry::categoryOf(NodeType::ApiService), NodeCategory::Compute);
    EXPECT_EQ(NodeRegistry::categoryOf(NodeType::Database), NodeCategory::Persistence);
    EXPECT_EQ(NodeRegistry::categoryOf(NodeType::Cache), NodeCategory::Acceleration);
    EXPECT_EQ(NodeRegistry::categoryOf(NodeType::WAF), NodeCategory::Security);
    EXPECT_TRUE(NodeRegistry::processesRequests(NodeType::ApiService));
    EXPECT_TRUE(NodeRegistry::storesState(NodeType::SecretVault));
}

TEST(LayerRegistryTests, RegistersSimulationLayerSkeletons)
{
    EXPECT_EQ(LayerRegistry::definitions().size(), 7U);
    EXPECT_EQ(LayerRegistry::definition(SimulationLayer::Flow).displayName, "Flow");
    EXPECT_EQ(LayerRegistry::definition(SimulationLayer::Resources).displayName, "Resources");
    EXPECT_TRUE(LayerRegistry::definition(SimulationLayer::Flow).enabledByDefault);
}

TEST(RuntimeSystemsTests, InitializesAndUpdatesEnabledSystems)
{
    Simulation simulation(Scenario::createDefault());

    simulation.update(1.0 / 60.0);

    EXPECT_EQ(simulation.runtimeSystems().states().size(), 7U);
    EXPECT_EQ(simulation.runtimeSystems().enabledCount(), 7);
    EXPECT_EQ(simulation.metrics().observability.enabledSystemCount, 7);
}

TEST(MechanicRegistryTests, RegistersInterventionDefinitions)
{
    EXPECT_EQ(MechanicRegistry::definitions().size(), static_cast<std::size_t>(MechanicType::Count));
    EXPECT_TRUE(MechanicRegistry::definition(MechanicType::ScaleUp).available);
    EXPECT_TRUE(MechanicRegistry::definition(MechanicType::EnableCache).available);
    EXPECT_EQ(MechanicRegistry::definition(MechanicType::AddReadReplica).affectedLayers[0], SimulationLayer::Persistence);
}

TEST(MechanicExecutorTests, AppliesCurrentlyImplementedMechanics)
{
    Simulation simulation(ScenarioRegistry::databaseBottleneck());
    MechanicExecutor executor;

    EXPECT_FALSE(simulation.cacheEnabled());
    executor.execute(simulation, {MechanicType::EnableCache});
    EXPECT_TRUE(simulation.cacheEnabled());

    executor.execute(simulation, {MechanicType::ThrottleTraffic, -1, 2.0});
    runFor(simulation, 1.1);
    EXPECT_GT(simulation.metrics().totalGenerated, 0U);
}

TEST(InterventionConstraintTests, ScaleUpTracksLevelsAndStopsAtCap)
{
    Simulation simulation(ScenarioRegistry::databaseBottleneck());

    ASSERT_TRUE(simulation.canScaleNode(-1, 2));
    EXPECT_TRUE(simulation.scaleApiCapacity(-1, 1.5, 2, 0.7, 1.0));
    EXPECT_TRUE(simulation.scaleApiCapacity(-1, 1.5, 2, 0.7, 1.0));
    EXPECT_FALSE(simulation.canScaleNode(-1, 2));
    EXPECT_FALSE(simulation.scaleApiCapacity(-1, 1.5, 2, 0.7, 1.0));
    EXPECT_DOUBLE_EQ(simulation.complexityScore(), 2.0);
}

TEST(InterventionConstraintTests, RegionSlotsRejectSaturatedTopologyExpansion)
{
    Simulation simulation(ScenarioRegistry::databaseBottleneck());
    PlacementCandidateGenerator generator;
    MutationValidator validator;
    TopologyBuilder builder;

    const auto candidates = generator.generate(simulation, TopologyMutationType::AddReadReplica);
    ASSERT_FALSE(candidates.empty());
    MutationPreview preview = validator.preview(simulation, TopologyMutationType::AddReadReplica, candidates.front());
    ASSERT_TRUE(preview.valid);
    preview.mutation.regionSlotUsage = 5;

    EXPECT_FALSE(builder.apply(simulation, preview.mutation));
}

TEST(TopologyMutationTests, GeneratesAndAppliesConstrainedCachePlacement)
{
    Simulation simulation(ScenarioRegistry::databaseBottleneck());
    PlacementCandidateGenerator generator;
    MutationValidator validator;
    TopologyBuilder builder;

    const auto candidates = generator.generate(simulation, TopologyMutationType::AddCache);
    ASSERT_FALSE(candidates.empty());
    const MutationPreview preview = validator.preview(simulation, TopologyMutationType::AddCache, candidates.front());

    ASSERT_TRUE(preview.valid);
    EXPECT_FALSE(preview.mutation.nodesToCreate.empty());
    EXPECT_FALSE(preview.mutation.linksToDisable.empty());
    EXPECT_TRUE(builder.apply(simulation, preview.mutation));

    bool hasCreatedCache = false;
    bool hasDisabledLink = false;
    for (const auto& node : simulation.graph().nodes()) {
        hasCreatedCache = hasCreatedCache || (node.type == NodeType::Cache && node.name.find("cache") != std::string::npos);
    }
    for (const auto& link : simulation.graph().links()) {
        hasDisabledLink = hasDisabledLink || !link.enabled;
    }
    EXPECT_TRUE(hasCreatedCache);
    EXPECT_TRUE(hasDisabledLink);
}

TEST(TopologyMutationTests, QueuePlacementIsRegionConstrained)
{
    Simulation simulation(ScenarioRegistry::burstTraffic());
    PlacementCandidateGenerator generator;
    const auto candidates = generator.generate(simulation, TopologyMutationType::AddQueue);

    ASSERT_FALSE(candidates.empty());
    for (const auto& candidate : candidates) {
        EXPECT_EQ(candidate.location.regionName, "Europe");
    }
}

TEST(ScenarioRegistryTests, ProvidesInitialScenarioSet)
{
    const auto scenarios = ScenarioRegistry::createAll();
    ASSERT_GE(scenarios.size(), 5U);
    const auto local = std::find_if(scenarios.begin(), scenarios.end(), [](const ScenarioDefinition& scenario) { return scenario.id == "local_startup"; });
    const auto database = std::find_if(scenarios.begin(), scenarios.end(), [](const ScenarioDefinition& scenario) { return scenario.id == "database_bottleneck"; });
    const auto burst = std::find_if(scenarios.begin(), scenarios.end(), [](const ScenarioDefinition& scenario) { return scenario.id == "burst_traffic"; });
    ASSERT_NE(local, scenarios.end());
    ASSERT_NE(database, scenarios.end());
    ASSERT_NE(burst, scenarios.end());
    EXPECT_EQ(local->name, "Single Service Overload");
    EXPECT_EQ(database->name, "Database Bottleneck");
    EXPECT_EQ(burst->name, "Burst Traffic");
    EXPECT_EQ(local->archetype, ScenarioArchetype::LocalStartup);
    EXPECT_EQ(database->minimumTier, ProgressionTier::StateAndCache);
    EXPECT_FALSE(local->phases.empty());
    EXPECT_FALSE(local->allowedMechanics.empty());
}

TEST(ScenarioRunTests, SeparatesStaticDefinitionFromSeededRun)
{
    auto scenario = ScenarioRegistry::databaseBottleneck();
    ScenarioManager first(scenario);
    ScenarioManager second(scenario);

    first.createRun(42);
    second.createRun(42);

    EXPECT_EQ(first.staticDefinition().optionalModifiers.size(), scenario.optionalModifiers.size());
    EXPECT_EQ(first.run().seed, 42U);
    EXPECT_EQ(first.run().selectedModifiers.size(), second.run().selectedModifiers.size());
    EXPECT_EQ(first.definition().minimumTier, ProgressionTier::StateAndCache);
}

TEST(ProgressionRegistryTests, ProvidesTierFilters)
{
    const auto& tier = ProgressionRegistry::definition(ProgressionTier::StateAndCache);
    EXPECT_EQ(tier.name, "State and Cache");
    EXPECT_FALSE(tier.visibleMetrics.empty());
    EXPECT_NE(std::find(tier.availableMechanics.begin(), tier.availableMechanics.end(), MechanicType::EnableCache), tier.availableMechanics.end());
    EXPECT_NE(std::find(tier.allowedPressures.begin(), tier.allowedPressures.end(), PressureCategory::PersistencePressure), tier.allowedPressures.end());
}

TEST(MetricsAggregatorTests, ComputesGlobalMetricsFromFrontendSignals)
{
    MetricsSnapshot snapshot;
    snapshot.averageLatencySeconds = 1.5;
    snapshot.inputRatePerSecond = 24.0;
    snapshot.processedPerSecond = 12.0;
    snapshot.retryRatePerSecond = 2.0;
    snapshot.timeoutRatePerSecond = 1.0;
    snapshot.apiQueueDepth = 4;
    snapshot.databaseQueueDepth = 6;
    snapshot.apiUtilization = 0.8;
    snapshot.databaseUtilization = 0.6;
    snapshot.cacheHitRate = 0.25;
    snapshot.totalCacheLookups = 10;
    snapshot.observability.enabledSystemCount = 7;
    snapshot.complexity.current = 2.0;
    snapshot.complexity.recommendedThreshold = 10.0;

    MetricsAggregator::update(snapshot);

    EXPECT_GT(snapshot.frontend.perceivedLatency, snapshot.averageLatencySeconds);
    EXPECT_GT(snapshot.frontend.framePressure, 0.0);
    EXPECT_LT(snapshot.global.userExperience, 100.0);
    EXPECT_GT(snapshot.global.infrastructurePressure, 0.0);
    EXPECT_LT(snapshot.global.reliability, 100.0);
    EXPECT_GT(snapshot.global.complexity, 20.0);
    EXPECT_FALSE(snapshot.contributions.empty());
}

TEST(MetricsAggregatorTests, HiddenPressureStateDerivesSpecializedMetrics)
{
    MetricsSnapshot snapshot;
    snapshot.averageLatencySeconds = 0.2;
    snapshot.inputRatePerSecond = 8.0;
    snapshot.processedPerSecond = 8.0;
    snapshot.pressureState.frontend.assetWeight = 0.9;
    snapshot.pressureState.frontend.renderComplexity = 0.8;
    snapshot.pressureState.frontend.realtimeIntensity = 0.7;
    snapshot.pressureState.frontend.sessionPersistence = 0.2;
    snapshot.pressureState.frontend.mobileCompatibility = 0.3;
    snapshot.pressureState.backend.serviceFragmentation = 0.6;
    snapshot.pressureState.network.bandwidthPressure = 0.7;
    snapshot.pressureState.network.trafficBurstiness = 0.8;
    snapshot.pressureState.database.readPressure = 0.7;
    snapshot.pressureState.database.contention = 0.8;
    snapshot.pressureState.database.replicationLag = 0.5;
    snapshot.pressureState.runtime.cpuPressure = 0.7;
    snapshot.pressureState.runtime.memoryPressure = 0.6;
    snapshot.pressureState.runtime.schedulingPressure = 0.5;

    MetricsAggregator::update(snapshot);

    EXPECT_GT(snapshot.frontend.assetBandwidth, 40.0);
    EXPECT_GT(snapshot.frontend.websocketPressure, 40.0);
    EXPECT_GT(snapshot.frontend.sessionStalenessRisk, 20.0);
    EXPECT_GT(snapshot.backend.serviceFragmentation, 50.0);
    EXPECT_GT(snapshot.network.deliveryPressure, 40.0);
    EXPECT_GT(snapshot.database.persistenceRisk, 40.0);
    EXPECT_GT(snapshot.runtime.executionRisk, 40.0);
    EXPECT_GT(snapshot.global.infrastructurePressure, 20.0);
    EXPECT_TRUE(std::any_of(snapshot.contributions.begin(), snapshot.contributions.end(), [](const MetricContribution& contribution) {
        return contribution.domain == MetricContributionDomain::Database;
    }));
    EXPECT_TRUE(std::any_of(snapshot.contributions.begin(), snapshot.contributions.end(), [](const MetricContribution& contribution) {
        return contribution.domain == MetricContributionDomain::Runtime;
    }));
}

TEST(CrossDomainInteractionTests, FrontendRealtimeCreatesBackendNetworkAndRuntimePressure)
{
    PressureState state;
    state.frontend.realtimeIntensity = 0.8;

    const PressureState propagated = CrossDomainInteraction::apply(state);

    EXPECT_GT(propagated.backend.requestLoad, state.backend.requestLoad);
    EXPECT_GT(propagated.network.bandwidthPressure, state.network.bandwidthPressure);
    EXPECT_GT(propagated.runtime.cpuPressure, state.runtime.cpuPressure);
}

TEST(CrossDomainInteractionTests, PropagationClampsWithoutRunawayValues)
{
    PressureState state;
    state.frontend.realtimeIntensity = 1.0;
    state.frontend.assetWeight = 1.0;
    state.backend.queuePressure = 1.0;
    state.backend.serviceFragmentation = 1.0;
    state.network.trafficBurstiness = 1.0;
    state.database.contention = 1.0;
    state.runtime.memoryPressure = 1.0;

    for (int i = 0; i < 100; ++i) {
        state = CrossDomainInteraction::apply(state);
    }

    EXPECT_LE(state.backend.requestLoad, 1.0);
    EXPECT_LE(state.backend.queuePressure, 1.0);
    EXPECT_LE(state.network.bandwidthPressure, 1.0);
    EXPECT_LE(state.runtime.schedulingPressure, 1.0);
    EXPECT_LE(state.database.replicationLag, 1.0);
}

TEST(CrossDomainInteractionTests, DatabaseContentionFlowsIntoFrontendLatencyThroughBackendQueues)
{
    PressureState state;
    state.database.contention = 0.8;

    MetricsSnapshot baseline;
    baseline.pressureState = state;
    MetricsAggregator::update(baseline);

    MetricsSnapshot propagated;
    propagated.pressureState = CrossDomainInteraction::apply(state);
    MetricsAggregator::update(propagated);

    EXPECT_GT(propagated.pressureState.backend.queuePressure, baseline.pressureState.backend.queuePressure);
    EXPECT_GT(propagated.frontend.perceivedLatency, baseline.frontend.perceivedLatency);
    EXPECT_LT(propagated.global.userExperience, baseline.global.userExperience);
}

TEST(ActionEffectTests, ActionPressureEffectCanModifyHiddenPressureState)
{
    Simulation simulation(ScenarioRegistry::databaseBottleneck());
    PressureState actionEffect;
    actionEffect.frontend.cacheEfficiency = 0.12;
    actionEffect.backend.serviceFragmentation = 0.08;
    actionEffect.database.readPressure = 0.10;
    actionEffect.runtime.cpuPressure = 0.09;

    simulation.applyPressureEffect(actionEffect);

    EXPECT_GT(simulation.metrics().pressureState.frontend.cacheEfficiency, 0.0);
    EXPECT_GT(simulation.metrics().frontend.sessionWarmth, 0.0);
    EXPECT_GT(simulation.metrics().database.readPressure, 0.0);
    EXPECT_GT(simulation.metrics().runtime.cpuPressure, 0.0);
    EXPECT_GT(simulation.metrics().global.complexity, 0.0);
}

TEST(EngineeringCapacityTests, SpecialtyIncreasesAreCappedAtTotalBudget)
{
    const EngineeringCapacity fullCapacity{
        .frontend = 1,
        .backend = 1,
        .infrastructure = 1,
        .data = 1,
        .total = 4,
    };

    const EngineeringCapacity cappedIncrease = applyEngineeringCapacityBudgetCap(fullCapacity, capacityBonus(0, 2, 0, 0, 0));
    EXPECT_EQ(cappedIncrease.backend, 1);
    EXPECT_EQ(specialtyCapacityTotal(cappedIncrease), cappedIncrease.total);

    const EngineeringCapacity rebalance = applyEngineeringCapacityBudgetCap(fullCapacity, capacityBonus(0, -1, 0, 1, 0));
    EXPECT_EQ(rebalance.backend, 0);
    EXPECT_EQ(rebalance.data, 2);
    EXPECT_EQ(specialtyCapacityTotal(rebalance), rebalance.total);

    const EngineeringCapacity budgetIncrease = applyEngineeringCapacityBudgetCap(fullCapacity, capacityBonus(0, 1, 0, 0, 1));
    EXPECT_EQ(budgetIncrease.backend, 2);
    EXPECT_EQ(budgetIncrease.total, 5);
    EXPECT_EQ(specialtyCapacityTotal(budgetIncrease), budgetIncrease.total);
}

TEST(ScenarioManagerTests, AppliesPhaseTrafficAndMechanicRestrictions)
{
    auto scenario = ScenarioRegistry::singleServiceOverload();
    ScenarioManager manager(scenario);
    Simulation simulation(scenario);

    manager.update(30.0, simulation);

    ASSERT_NE(manager.currentPhase(), nullptr);
    EXPECT_EQ(manager.currentPhase()->name, "Demand exceeds API");
    EXPECT_TRUE(simulation.isMechanicAllowed(MechanicType::ScaleUp));
    EXPECT_FALSE(simulation.isMechanicAllowed(MechanicType::EnableCache));
}

TEST(EventManagerTests, ActivatesTimeBasedEvents)
{
    auto scenario = ScenarioRegistry::burstTraffic();
    ScenarioManager manager(scenario);
    Simulation simulation(scenario);

    manager.update(27.0, simulation);

    EXPECT_FALSE(manager.eventManager().recentEvents().empty());
    EXPECT_GT(manager.eventManager().activeEvents().size(), 0U);
}

TEST(SimulationTimeTests, TracksElapsedScenarioAndPhaseTime)
{
    Simulation simulation(ScenarioRegistry::singleServiceOverload());
    simulation.update(1.0);
    simulation.setScenarioTime(12.0, 4.0);

    EXPECT_GE(simulation.timeState().elapsedSeconds, 1.0);
    EXPECT_EQ(simulation.timeState().scenarioElapsedSeconds, 12.0);
    EXPECT_EQ(simulation.timeState().phaseElapsedSeconds, 4.0);
}

TEST(PressureAnalysisTests, TracksBottlenecksAndHints)
{
    Simulation simulation(ScenarioRegistry::databaseBottleneck());
    runFor(simulation, 4.0);

    EXPECT_GE(simulation.pressure().topOverloadedNodeId, 0);
    EXPECT_GE(simulation.pressure().dominantLatencyNodeId, 0);
    EXPECT_FALSE(simulation.pressure().nodes.empty());
}

TEST(GeographyTests, ProjectsCoordinatesIndependentlyOfMapTexture)
{
    const Vec2 greenwich = MapProjection::projectEquirectangular({0.0, 0.0, "Europe"});
    EXPECT_NEAR(greenwich.x, 0.0f, 0.01f);
    EXPECT_NEAR(greenwich.y, 0.0f, 0.01f);

    const Vec2 northAmerica = MapProjection::projectEquirectangular({37.77, -122.42, "NorthAmerica"});
    const Vec2 europe = MapProjection::projectEquirectangular({50.11, 8.68, "Europe"});
    EXPECT_LT(northAmerica.x, europe.x);
    EXPECT_LT(europe.y, northAmerica.y);
}

TEST(GeographyTests, DistanceLatencyIncreasesAcrossRegions)
{
    const GeographicSystem geography;
    const GeoLocation europeApi{50.11, 8.68, "Europe"};
    const GeoLocation europeDb{50.12, 8.67, "Europe"};
    const GeoLocation northAmericaUsers{37.77, -122.42, "NorthAmerica"};

    const double localLatency = geography.latencySeconds(europeApi, europeDb, 0.1);
    const double intercontinentalLatency = geography.latencySeconds(northAmericaUsers, europeApi, 0.1);
    EXPECT_GT(intercontinentalLatency, localLatency);
}

TEST(GeographyTests, DefaultScenarioBuildsGeographicTopology)
{
    Simulation simulation(Scenario::createDefault());
    const Node* naUsers = simulation.graph().node(0);
    const Node* api = simulation.graph().node(3);
    ASSERT_NE(naUsers, nullptr);
    ASSERT_NE(api, nullptr);

    EXPECT_TRUE(naUsers->hasGeoLocation);
    EXPECT_TRUE(api->hasGeoLocation);
    EXPECT_EQ(naUsers->geoLocation.regionName, "NorthAmerica");
    EXPECT_EQ(api->geoLocation.regionName, "Europe");
    EXPECT_FALSE(api->networkIdentity.hostname.empty());
    EXPECT_GT(simulation.graph().links().front().geographicLatencyContributionSeconds, 0.0);
}

TEST(GeographyTests, RegionalDemandEventCanAddNewDemandArea)
{
    Simulation simulation(Scenario::createDefault());
    const auto nodeCount = simulation.graph().nodes().size();
    const auto linkCount = simulation.graph().links().size();

    ASSERT_TRUE(simulation.addRegionalDemandSource({.scope = EventLocationScope::Region, .region = "Africa"}, 1.25));

    EXPECT_EQ(simulation.graph().nodes().size(), nodeCount + 1);
    EXPECT_EQ(simulation.graph().links().size(), linkCount + 1);
    const Node& node = simulation.graph().nodes().back();
    EXPECT_EQ(node.type, NodeType::ClientCluster);
    EXPECT_EQ(node.geoLocation.regionName, "Africa");
    EXPECT_DOUBLE_EQ(node.requestRatePerSecond, 1.25);
}

ScenarioDefinition saturatedScenario()
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

ScenarioDefinition databasePressureScenario()
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

ScenarioDefinition adaptiveTrafficScenario()
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
}

TEST(SimulationTests, DefaultScenarioGeneratesAndCompletesRequests)
{
    Simulation simulation(Scenario::createDefault());

    runFor(simulation, 4.0);

    const auto& metrics = simulation.metrics();
    EXPECT_GT(metrics.totalGenerated, 0U);
    EXPECT_GT(metrics.totalProcessed, 0U);
    EXPECT_GT(metrics.averageLatencySeconds, 0.0);
}

TEST(SimulationTests, HiddenPressureStateEvolvesUnderBurstLoad)
{
    Simulation simulation(ScenarioRegistry::burstTraffic());
    simulation.toggleBurstMode();

    runFor(simulation, 2.0);

    const auto& metrics = simulation.metrics();
    EXPECT_GT(metrics.pressureState.backend.requestLoad, 0.0);
    EXPECT_GT(metrics.pressureState.network.trafficBurstiness, 0.0);
    EXPECT_GT(metrics.backend.requestLoad, 0.0);
    EXPECT_GT(metrics.network.trafficBurstiness, 0.0);
}

TEST(SimulationTests, ScenarioPressureContextFeedsSpecializedAndGlobalMetrics)
{
    auto scenario = ScenarioRegistry::singleServiceOverload();
    scenario.pressureContext.frontend.assetWeight = 0.35;
    scenario.pressureContext.frontend.renderComplexity = 0.25;
    scenario.pressureContext.network.bandwidthPressure = 0.2;
    scenario.pressureSignals.push_back({
        .domain = MetricContributionDomain::Frontend,
        .name = "Mobile Audience Surge",
        .summary = "Mobile users amplify asset and rendering pressure.",
    });

    Simulation simulation(scenario);
    runFor(simulation, 1.0);

    const auto& metrics = simulation.metrics();
    EXPECT_GT(metrics.pressureState.frontend.assetWeight, 0.0);
    EXPECT_GT(metrics.frontend.assetBandwidth, 0.0);
    EXPECT_LT(metrics.global.userExperience, 100.0);
    ASSERT_FALSE(metrics.activePressureSignals.empty());
    EXPECT_EQ(metrics.activePressureSignals.front().name, "Mobile Audience Surge");
}

TEST(SimulationTests, EventPressureContextTemporarilyAmplifiesMetrics)
{
    Simulation simulation(ScenarioRegistry::singleServiceOverload());
    EventManager events;
    EventDefinition event;
    event.id = "network_instability_test";
    event.displayName = "Network Instability";
    event.trigger.timeSeconds = 0.0;
    event.durationSeconds = 0.1;
    event.durationTurns = 0;
    event.effect.pressureEffect.network.latencySensitivity = 0.35;
    event.effect.pressureEffect.network.trafficBurstiness = 0.25;
    event.effect.pressureSignals.push_back({
        .domain = MetricContributionDomain::Runtime,
        .name = "Network Instability",
        .summary = "Latency-sensitive traffic makes connection choices more visible.",
        .temporary = true,
    });
    events.reset({event});

    events.update(0.01, 0.01, 1, 1.0, -1, simulation);
    simulation.setEventPressureContext(events.modifiers().pressureEffect, events.modifiers().pressureSignals);
    runFor(simulation, 1.0);

    const auto& activeMetrics = simulation.metrics();
    EXPECT_GT(activeMetrics.network.latencySensitivity, 0.0);
    ASSERT_FALSE(activeMetrics.activePressureSignals.empty());
    EXPECT_TRUE(activeMetrics.activePressureSignals.front().temporary);

    events.update(0.2, 0.21, 1, 1.0, -1, simulation);
    simulation.setEventPressureContext(events.modifiers().pressureEffect, events.modifiers().pressureSignals);
    simulation.update(1.0 / 60.0);

    EXPECT_TRUE(simulation.metrics().activePressureSignals.empty());
}

TEST(SimulationTests, QueueBuildsWhenDemandExceedsCapacity)
{
    Simulation simulation(saturatedScenario());

    runFor(simulation, 1.2);

    EXPECT_GT(simulation.metrics().apiQueueDepth, 0);
    EXPECT_GT(simulation.metrics().apiUtilization, 0.0);
}

TEST(SimulationTests, RequestsTimeOutUnderSustainedOverload)
{
    Simulation simulation(saturatedScenario());

    runFor(simulation, 4.0);

    EXPECT_GT(simulation.metrics().totalTimedOut, 0U);
}

TEST(SimulationTests, DatabaseCanBottleneckIndependentlyOfApi)
{
    Simulation simulation(databasePressureScenario());
    simulation.scaleApiCapacity(3.0);

    runFor(simulation, 2.0);

    EXPECT_GT(simulation.metrics().databaseQueueDepth, 0);
    EXPECT_GT(simulation.metrics().databaseUtilization, 0.0);
}

TEST(SimulationTests, RetriesAddLoadAfterTimeouts)
{
    Simulation simulation(databasePressureScenario());

    runFor(simulation, 5.0);

    EXPECT_GT(simulation.metrics().totalTimedOut, 0U);
    EXPECT_GT(simulation.metrics().totalRetries, 0U);
}

TEST(SimulationTests, EnabledCacheProducesHitsForRepeatedDatabaseRequests)
{
    auto scenario = databasePressureScenario();
    scenario.nodes[0].requestRatePerSecond = 4.0;
    scenario.nodes[2].processingCapacityPerSecond = 20.0;
    scenario.cache.enabled = true;
    scenario.cache.maxEntries = 12;
    scenario.cache.ttlSeconds = 30.0;

    Simulation cached(scenario);
    runFor(cached, 6.0);

    EXPECT_GT(cached.metrics().totalCacheLookups, 0U);
    EXPECT_GT(cached.metrics().totalCacheHits, 0U);
}

TEST(SimulationTests, AdvancedTrafficReroutesTowardBetterIngressPath)
{
    Simulation adaptive(adaptiveTrafficScenario());

    runFor(adaptive, 0.75);

    EXPECT_GT(adaptive.metrics().totalProcessed, 0U);
    ASSERT_GE(adaptive.graph().nodes().size(), 3U);
    EXPECT_EQ(adaptive.graph().nodes()[1].queue.size(), 0U);
}
