#include "gameplay/Scenario.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "simulation/Mechanics.hpp"
#include "simulation/NodeDefinition.hpp"
#include "simulation/Simulation.hpp"

#include <gtest/gtest.h>

namespace {
void runFor(Simulation& simulation, double seconds)
{
    constexpr double fixedStep = 1.0 / 60.0;
    const int steps = static_cast<int>(seconds / fixedStep);
    for (int i = 0; i < steps; ++i) {
        simulation.update(fixedStep);
    }
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

TEST(ScenarioRegistryTests, ProvidesInitialScenarioSet)
{
    const auto scenarios = ScenarioRegistry::createAll();
    ASSERT_EQ(scenarios.size(), 3U);
    EXPECT_EQ(scenarios[0].name, "Single Service Overload");
    EXPECT_EQ(scenarios[1].name, "Database Bottleneck");
    EXPECT_EQ(scenarios[2].name, "Burst Traffic");
    EXPECT_FALSE(scenarios[0].phases.empty());
    EXPECT_FALSE(scenarios[0].allowedMechanics.empty());
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
