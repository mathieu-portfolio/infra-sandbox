#include "gameplay/Scenario.hpp"
#include "simulation/core/Simulation.hpp"
#include "support/TestHelpers.hpp"

#include <gtest/gtest.h>

using test_support::adaptiveTrafficScenario;
using test_support::databasePressureScenario;
using test_support::runFor;
using test_support::saturatedScenario;

TEST(RuntimeSystemsTests, InitializesAndUpdatesEnabledSystems)
{
    Simulation simulation(Scenario::createDefault());

    simulation.update(1.0 / 60.0);

    EXPECT_EQ(simulation.runtimeSystems().states().size(), 7U);
    EXPECT_EQ(simulation.runtimeSystems().enabledCount(), 7);
    EXPECT_EQ(simulation.metrics().observability.enabledSystemCount, 7);
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

TEST(SimulationFlowTests, DefaultScenarioGeneratesAndCompletesRequests)
{
    Simulation simulation(Scenario::createDefault());

    runFor(simulation, 4.0);

    const auto& metrics = simulation.metrics();
    EXPECT_GT(metrics.totalGenerated, 0U);
    EXPECT_GT(metrics.totalProcessed, 0U);
    EXPECT_GT(metrics.averageLatencySeconds, 0.0);
}

TEST(SimulationFlowTests, QueueBuildsWhenDemandExceedsCapacity)
{
    Simulation simulation(saturatedScenario());

    runFor(simulation, 1.2);

    EXPECT_GT(simulation.metrics().apiQueueDepth, 0);
    EXPECT_GT(simulation.metrics().apiUtilization, 0.0);
}

TEST(SimulationFlowTests, RequestsTimeOutUnderSustainedOverload)
{
    Simulation simulation(saturatedScenario());

    runFor(simulation, 4.0);

    EXPECT_GT(simulation.metrics().totalTimedOut, 0U);
}

TEST(SimulationFlowTests, DatabaseCanBottleneckIndependentlyOfApi)
{
    Simulation simulation(databasePressureScenario());
    simulation.scaleApiCapacity(3.0);

    runFor(simulation, 2.0);

    EXPECT_GT(simulation.metrics().databaseQueueDepth, 0);
    EXPECT_GT(simulation.metrics().databaseUtilization, 0.0);
}

TEST(SimulationFlowTests, RetriesAddLoadAfterTimeouts)
{
    Simulation simulation(databasePressureScenario());

    runFor(simulation, 5.0);

    EXPECT_GT(simulation.metrics().totalTimedOut, 0U);
    EXPECT_GT(simulation.metrics().totalRetries, 0U);
}

TEST(SimulationFlowTests, EnabledCacheProducesHitsForRepeatedDatabaseRequests)
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

TEST(SimulationFlowTests, AdvancedTrafficReroutesTowardBetterIngressPath)
{
    Simulation adaptive(adaptiveTrafficScenario());

    runFor(adaptive, 0.75);

    EXPECT_GT(adaptive.metrics().totalProcessed, 0U);
    ASSERT_GE(adaptive.graph().nodes().size(), 3U);
    EXPECT_EQ(adaptive.graph().nodes()[1].queue.size(), 0U);
}
