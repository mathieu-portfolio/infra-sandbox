#pragma once

#include "simulation/topology/Link.hpp"
#include "simulation/topology/Node.hpp"

#include <string>
#include <vector>

class Simulation;

enum class TopologyMutationType {
    AddCache,
    AddReadReplica,
    AddQueue,
    AddRegionalCache
};

struct PlacementOption {
    std::string id;
    std::string displayName;
    GeoLocation location;
    std::string latencyImpact;
    std::string trafficImpact;
    std::string resourceCost;
    std::string complexityImpact;
};

struct TopologyMutation {
    TopologyMutationType type = TopologyMutationType::AddCache;
    PlacementOption placement;
    std::vector<Node> nodesToCreate;
    std::vector<Link> linksToCreate;
    std::vector<int> linksToDisable;
    std::string description;
    double complexityCost = 0.0;
    int regionSlotUsage = 0;
};

struct MutationPreview {
    bool valid = false;
    std::string validationMessage;
    TopologyMutation mutation;
};

class PlacementCandidateGenerator {
public:
    [[nodiscard]] std::vector<PlacementOption> generate(const Simulation& simulation, TopologyMutationType type) const;
};

class MutationValidator {
public:
    [[nodiscard]] MutationPreview preview(const Simulation& simulation, TopologyMutationType type, const PlacementOption& option) const;
};

class TopologyBuilder {
public:
    bool apply(Simulation& simulation, const TopologyMutation& mutation) const;
};

const char* topologyMutationName(TopologyMutationType type);
