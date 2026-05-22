#include "gameplay/Scenario.hpp"
#include "simulation/core/Simulation.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <gtest/gtest.h>

#include <string>

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

TEST(TopologyMutationTests, QueuePlacementCandidatesAreRegionScoped)
{
    Simulation simulation(ScenarioRegistry::burstTraffic());
    PlacementCandidateGenerator generator;
    MutationValidator validator;
    const auto candidates = generator.generate(simulation, TopologyMutationType::AddQueue);

    ASSERT_FALSE(candidates.empty());
    bool includesEurope = false;
    for (const auto& candidate : candidates) {
        EXPECT_FALSE(candidate.location.regionName.empty());
        includesEurope = includesEurope || candidate.location.regionName == "Europe";

        const MutationPreview preview = validator.preview(simulation, TopologyMutationType::AddQueue, candidate);
        EXPECT_TRUE(preview.valid) << preview.validationMessage;
        ASSERT_FALSE(preview.mutation.nodesToCreate.empty());
        EXPECT_EQ(preview.mutation.nodesToCreate.front().type, NodeType::QueueBroker);
        EXPECT_EQ(preview.mutation.nodesToCreate.front().geoLocation.regionName, candidate.location.regionName);
    }
    EXPECT_TRUE(includesEurope);
}
