#include "core/topology/Geography.hpp"
#include "gameplay/Scenario.hpp"
#include "simulation/core/Simulation.hpp"

#include <gtest/gtest.h>

TEST(GeographyTests, ProjectsCoordinatesIndependentlyOfMapTexture)
{
    const Vec2 greenwich = MapProjection::projectEquirectangular({0.0, 0.0, "Europe"});
    EXPECT_NEAR(greenwich.x, 0.0f, 0.01f);
    EXPECT_NEAR(greenwich.y, 0.0f, 0.01f);

    const Vec2 northAmerica = MapProjection::projectEquirectangular({37.77, -122.42, "NorthAmerica"});
    const Vec2 europe = MapProjection::projectEquirectangular({50.11, 8.68, "Europe"});
    EXPECT_LT(northAmerica.x, europe.x);
    EXPECT_LT(europe.y, northAmerica.y);
}

TEST(GeographyTests, DistanceLatencyIncreasesAcrossRegions)
{
    const GeographicSystem geography;
    const GeoLocation europeApi{50.11, 8.68, "Europe"};
    const GeoLocation europeDb{50.12, 8.67, "Europe"};
    const GeoLocation northAmericaUsers{37.77, -122.42, "NorthAmerica"};

    const double localLatency = geography.latencySeconds(europeApi, europeDb, 0.1);
    const double intercontinentalLatency = geography.latencySeconds(northAmericaUsers, europeApi, 0.1);
    EXPECT_GT(intercontinentalLatency, localLatency);
}

TEST(GeographyTests, DefaultScenarioBuildsGeographicTopology)
{
    Simulation simulation(Scenario::createDefault());
    const Node* naUsers = simulation.graph().node(0);
    const Node* api = simulation.graph().node(3);
    ASSERT_NE(naUsers, nullptr);
    ASSERT_NE(api, nullptr);

    EXPECT_TRUE(naUsers->hasGeoLocation);
    EXPECT_TRUE(api->hasGeoLocation);
    EXPECT_EQ(naUsers->geoLocation.regionName, "NorthAmerica");
    EXPECT_EQ(api->geoLocation.regionName, "Europe");
    EXPECT_FALSE(api->networkIdentity.hostname.empty());
    EXPECT_GT(simulation.graph().links().front().geographicLatencyContributionSeconds, 0.0);
}

TEST(GeographyTests, RegionalDemandEventCanAddNewDemandArea)
{
    Simulation simulation(Scenario::createDefault());
    const auto nodeCount = simulation.graph().nodes().size();
    const auto linkCount = simulation.graph().links().size();

    ASSERT_TRUE(simulation.addRegionalDemandSource({.scope = EventLocationScope::Region, .region = "Africa"}, 1.25));

    EXPECT_EQ(simulation.graph().nodes().size(), nodeCount + 1);
    EXPECT_EQ(simulation.graph().links().size(), linkCount + 1);
    const Node& node = simulation.graph().nodes().back();
    EXPECT_EQ(node.type, NodeType::ClientCluster);
    EXPECT_EQ(node.geoLocation.regionName, "Africa");
    EXPECT_DOUBLE_EQ(node.requestRatePerSecond, 1.25);
}
