#include "content/ContentRegistry.hpp"
#include "core/topology/NodeDefinition.hpp"
#include "gameplay/Scenario.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>

TEST(ContentRegistryTests, LoadsTechIndustryNodeCatalogScenario)
{
    const std::filesystem::path contentRoot = INFRA_CONTENT_DIR;
    auto result = content::ContentRegistry::instance().loadFromLayers({
        contentRoot / "common",
        contentRoot / "packs" / "vanilla",
    });

    std::string errors;
    for (const auto& error : result.errors) {
        errors += error + "\n";
    }
    ASSERT_TRUE(result.loaded) << errors;
    const auto& scenarios = content::ContentRegistry::instance().scenarios();
    const auto scenario = std::find_if(scenarios.begin(), scenarios.end(), [](const ScenarioDefinition& entry) {
        return entry.id == "tech_industry_nodes_lab";
    });
    ASSERT_NE(scenario, scenarios.end());

    const auto hasNode = [&](const std::string& id, NodeType type) {
        return std::any_of(scenario->nodes.begin(), scenario->nodes.end(), [&](const NodeScenario& node) {
            return node.id == id && node.type == type;
        });
    };

    EXPECT_TRUE(hasNode("cdn", NodeType::CDNEdge));
    EXPECT_TRUE(hasNode("dns_resolver", NodeType::ServiceRegistry));
    EXPECT_TRUE(hasNode("ddos_protection", NodeType::WAF));
    EXPECT_TRUE(hasNode("kubernetes_cluster", NodeType::Orchestrator));
    EXPECT_TRUE(hasNode("identity_provider", NodeType::AuthService));
    EXPECT_TRUE(hasNode("graphql_gateway", NodeType::Gateway));
    EXPECT_TRUE(hasNode("service_mesh", NodeType::Proxy));
    EXPECT_TRUE(hasNode("timeseries_database", NodeType::MetricsCollector));
    EXPECT_TRUE(hasNode("feature_flag_service", NodeType::ServiceRegistry));
    EXPECT_TRUE(hasNode("ci_cd_pipeline", NodeType::Orchestrator));
    EXPECT_TRUE(hasNode("etl_pipeline", NodeType::BatchProcessor));
    EXPECT_TRUE(hasNode("data_warehouse", NodeType::StorageCluster));
    EXPECT_TRUE(hasNode("distributed_lock_service", NodeType::ConsensusNode));
    EXPECT_TRUE(hasNode("object_storage", NodeType::ObjectStorage));
    EXPECT_TRUE(hasNode("ai_inference_gateway", NodeType::AIInferenceNode));
    EXPECT_TRUE(hasNode("vector_database", NodeType::VectorDatabase));
}
