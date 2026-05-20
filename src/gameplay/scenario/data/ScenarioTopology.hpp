#pragma once

#include "simulation/core/Mechanics.hpp"
#include "simulation/topology/Geography.hpp"
#include "simulation/topology/InfrastructureGraph.hpp"

#include <optional>
#include <string>

struct ResourceProfile {
    double compute = 1.0;
    double memory = 0.0;
    double storage = 0.0;
    double network = 0.0;
};

struct NodeScenario {
    std::string id;
    std::string name;
    NodeType type = NodeType::ApiService;
    Vec2 position{};
    std::optional<GeoLocation> geoLocation;
    NetworkIdentity networkIdentity;
    double requestRatePerSecond = 0.0;
    double processingCapacityPerSecond = 0.0;
    double timeoutSeconds = 6.0;
    ResourceProfile resourceProfile;
};

struct LinkScenario {
    int sourceNode = 0;
    int targetNode = 0;
    double baseLatencySeconds = 0.6;
    double bandwidthPerSecond = 100.0;
};
