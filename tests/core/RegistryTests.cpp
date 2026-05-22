#include "core/topology/NodeDefinition.hpp"
#include "simulation/core/Simulation.hpp"

#include <gtest/gtest.h>

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
