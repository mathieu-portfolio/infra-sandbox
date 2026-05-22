#include "gameplay/Scenario.hpp"
#include "simulation/core/Simulation.hpp"
#include "support/TestHelpers.hpp"

#include <gtest/gtest.h>

using test_support::runFor;

TEST(PressureAnalysisTests, TracksBottlenecksAndHints)
{
    Simulation simulation(ScenarioRegistry::databaseBottleneck());
    runFor(simulation, 4.0);

    EXPECT_GE(simulation.pressure().topOverloadedNodeId, 0);
    EXPECT_GE(simulation.pressure().dominantLatencyNodeId, 0);
    EXPECT_FALSE(simulation.pressure().nodes.empty());
}

TEST(SimulationPressureTests, HiddenPressureStateEvolvesUnderBurstLoad)
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

TEST(SimulationPressureTests, ScenarioPressureContextFeedsSpecializedAndGlobalMetrics)
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
