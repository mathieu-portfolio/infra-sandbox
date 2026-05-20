#pragma once

#include "input/InputAction.hpp"
#include "rendering/CameraController.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "simulation/core/Mechanics.hpp"
#include "simulation/core/Simulation.hpp"
#include "simulation/topology/TopologyMutation.hpp"
#include "ui/core/UiTypes.hpp"

#include <span>
#include <string>

class InterventionController {
public:
    void handleActions(std::span<const InputEvent> events, Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const CameraController& camera);

private:
    void startPlacement(const Simulation& simulation, UiState& uiState, TopologyMutationType type) const;
    void moveCandidate(const Simulation& simulation, UiState& uiState, int delta) const;
    void confirmPlacement(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const PlacementOption& option) const;
    void confirmHoveredPlacement(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const CameraController& camera, Vector2 mousePosition) const;
    void handleActionPanelClick(const InputEvent& event, Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState) const;
    void executeMechanic(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const MechanicCommand& command, std::string actionName, std::string target) const;
    void recordFeedback(UiState& uiState, const Simulation& simulation, std::string actionName, std::string target, std::string message) const;
    void queueMechanic(const Simulation& simulation, UiState& uiState, const MechanicCommand& command, std::string actionName, std::string target) const;
    void queueTopologyMutation(const Simulation& simulation, UiState& uiState, const TopologyMutation& mutation, TopologyMutationType type, std::string actionName, std::string target, std::string preview) const;

    MechanicExecutor mechanicExecutor_;
    PlacementCandidateGenerator candidateGenerator_;
    MutationValidator mutationValidator_;
    TopologyBuilder topologyBuilder_;
};
