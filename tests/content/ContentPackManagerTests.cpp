#include "content/loading/ContentPackManager.hpp"
#include "support/TestHelpers.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using test_support::joinedErrors;

TEST(ContentPackManagerTests, DiscoveryHidesIncompleteSelectablePacks)
{
    const std::filesystem::path contentRoot = INFRA_CONTENT_DIR;
    content::ContentPackManager packManager;
    const auto result = packManager.discover(contentRoot);

    ASSERT_TRUE(result.loaded) << joinedErrors(result);
    EXPECT_NE(packManager.packById("ai_infrastructure"), nullptr);
    EXPECT_NE(packManager.packById("platform_ops"), nullptr);
    EXPECT_NE(packManager.packById("security_ops"), nullptr);
    EXPECT_NE(packManager.packById("networking_focus"), nullptr);
}

TEST(ContentPackManagerTests, ProbeLoadsEverySelectablePack)
{
    const std::filesystem::path contentRoot = INFRA_CONTENT_DIR;
    content::ContentPackManager packManager;
    const auto discovery = packManager.discover(contentRoot);

    ASSERT_TRUE(discovery.loaded) << joinedErrors(discovery);
    for (const auto& pack : packManager.packs()) {
        const auto result = packManager.loadPack(pack.metadata.id);
        ASSERT_TRUE(result.loaded) << "Pack " << pack.metadata.id << " failed to load:\n" << joinedErrors(result);
    }
}
