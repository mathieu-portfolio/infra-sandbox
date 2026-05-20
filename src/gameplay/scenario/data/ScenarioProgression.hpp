#pragma once

#include "gameplay/scenario/data/ScenarioEnums.hpp"
#include "simulation/core/Mechanics.hpp"
#include "simulation/metrics/PressureAnalysis.hpp"
#include "simulation/topology/InfrastructureGraph.hpp"

#include <set>
#include <string>
#include <vector>

struct ProgressionTierDefinition {
    std::string id;
    std::string displayName;
    std::string description;
    std::vector<std::string> tags;
    ProgressionTier tier = ProgressionTier::Foundations;
    std::string name;
    std::vector<std::string> visibleMetrics;
    std::vector<MechanicType> availableMechanics;
    std::vector<PressureCategory> allowedPressures;
    std::vector<NodeType> allowedNodeTypes;
};

struct ProgressionState {
    std::set<std::string> completedScenarios;
    std::set<std::string> unlockedScenarios;
    std::set<std::string> unlockedConcepts;
    std::set<std::string> unlockedMetrics;
    std::set<std::string> unlockedOverlays;
};

class ProgressionRegistry {
public:
    [[nodiscard]] static const std::vector<ProgressionTierDefinition>& definitions();
    [[nodiscard]] static const ProgressionTierDefinition& definition(ProgressionTier tier);
};
