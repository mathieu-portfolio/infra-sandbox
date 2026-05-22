#include "gameplay/Scenario.hpp"
#include "gameplay/scenario/ScenarioManager.hpp"
#include "simulation/core/Simulation.hpp"

#include <gtest/gtest.h>

#include <algorithm>

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
