#pragma once

#include "input/InputAction.hpp"
#include "simulation/Mechanics.hpp"
#include "simulation/Simulation.hpp"
#include "simulation/TopologyMutation.hpp"
#include "ui/UiTypes.hpp"

#include <span>

class InterventionController {
public:
    void handleActions(std::span<const InputEvent> events, Simulation& simulation, UiState& uiState);

private:
    void startPlacement(const Simulation& simulation, UiState& uiState, TopologyMutationType type) const;
    void moveCandidate(const Simulation& simulation, UiState& uiState, int delta) const;
    void confirmPlacement(Simulation& simulation, UiState& uiState) const;

    MechanicExecutor mechanicExecutor_;
    PlacementCandidateGenerator candidateGenerator_;
    MutationValidator mutationValidator_;
    TopologyBuilder topologyBuilder_;
};
