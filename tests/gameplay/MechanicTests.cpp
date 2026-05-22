#include "core/simulation/Mechanics.hpp"
#include "gameplay/Scenario.hpp"
#include "simulation/core/Simulation.hpp"
#include "support/TestHelpers.hpp"

#include <gtest/gtest.h>

using test_support::capacityBonus;
using test_support::runFor;

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
