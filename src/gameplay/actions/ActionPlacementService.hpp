#pragma once

#include "gameplay/actions/ActionQueue.hpp"
#include "gameplay/scenario/ScenarioManager.hpp"
#include "simulation/core/Simulation.hpp"
#include "simulation/topology/TopologyMutation.hpp"
#include "ui/core/UiTypes.hpp"

namespace gameplay::actions {

class ActionPlacementService {
public:
    void startPlacement(const Simulation& simulation, UiState& uiState, TopologyMutationType type) const;
    void moveCandidate(const Simulation& simulation, UiState& uiState, int delta) const;
    void confirmPlacement(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const PlacementOption& option) const;
};

}
