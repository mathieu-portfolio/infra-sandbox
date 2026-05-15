#include "gameplay/Scenario.hpp"
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

ScenarioDefinition saturatedScenario()
{
    ScenarioDefinition scenario;
    scenario.name = "Saturated backend test";
    scenario.nodes = {
        {.name = "Clients", .type = NodeType::ClientCluster, .position = {-100.0f, 0.0f}, .requestRatePerSecond = 12.0},
        {.name = "API", .type = NodeType::Service, .position = {100.0f, 0.0f}, .processingCapacityPerSecond = 1.0, .timeoutSeconds = 1.5},
    };
    scenario.links = {
        {.sourceNode = 0, .targetNode = 1, .baseLatencySeconds = 0.1, .bandwidthPerSecond = 100.0},
    };
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

    EXPECT_GT(simulation.metrics().backendQueueDepth, 0);
    EXPECT_GT(simulation.metrics().backendUtilization, 0.0);
}

TEST(SimulationTests, RequestsTimeOutUnderSustainedOverload)
{
    Simulation simulation(saturatedScenario());

    runFor(simulation, 4.0);

    EXPECT_GT(simulation.metrics().totalTimedOut, 0U);
}

TEST(SimulationTests, CachePlaceholderReducesEffectiveInputRate)
{
    Simulation baseline(Scenario::createDefault());
    runFor(baseline, 2.2);

    Simulation cached(Scenario::createDefault());
    cached.applyCachePlaceholder();
    cached.applyCachePlaceholder();
    runFor(cached, 2.2);

    EXPECT_LT(cached.metrics().totalGenerated, baseline.metrics().totalGenerated);
}
