#include "gameplay/Scenario.hpp"
#include "gameplay/scenario/ScenarioManager.hpp"
#include "simulation/core/Simulation.hpp"
#include "support/TestHelpers.hpp"

#include <gtest/gtest.h>

using test_support::runFor;

TEST(EventManagerTests, ActivatesTimeBasedEvents)
{
    auto scenario = ScenarioRegistry::burstTraffic();
    ScenarioManager manager(scenario);
    Simulation simulation(scenario);

    manager.update(27.0, simulation);

    EXPECT_FALSE(manager.eventManager().recentEvents().empty());
    EXPECT_GT(manager.eventManager().activeEvents().size(), 0U);
}

TEST(EventManagerTests, EventPressureContextTemporarilyAmplifiesMetrics)
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
