#pragma once

#include "input/InputAction.hpp"
#include "rendering/CameraController.hpp"
#include "gameplay/scenario/ScenarioManager.hpp"
#include "simulation/core/Simulation.hpp"
#include "simulation/topology/TopologyMutation.hpp"
#include "ui/core/UiTypes.hpp"

#include <span>

class InterventionController {
public:
    void handleActions(std::span<const InputEvent> events, Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const CameraController& camera);

private:
    void handleActionPanelClick(const InputEvent& event, Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState) const;
};
