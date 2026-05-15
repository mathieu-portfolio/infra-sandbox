#pragma once

#include "simulation/InfrastructureGraph.hpp"

#include <string>
#include <vector>

struct NodeScenario {
    std::string name;
    NodeType type = NodeType::Service;
    Vec2 position{};
    double requestRatePerSecond = 0.0;
    double processingCapacityPerSecond = 0.0;
    double timeoutSeconds = 6.0;
};

struct LinkScenario {
    int sourceNode = 0;
    int targetNode = 0;
    double baseLatencySeconds = 0.6;
    double bandwidthPerSecond = 100.0;
};

struct ScenarioDefinition {
    std::string name;
    std::vector<NodeScenario> nodes;
    std::vector<LinkScenario> links;
};

class Scenario {
public:
    static ScenarioDefinition createDefault();
};
