#pragma once

#include "input/InputAction.hpp"
#include "rendering/CameraController.hpp"
#include "simulation/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

#include <span>

class SelectionController {
public:
    void handleActions(std::span<const InputEvent> events, const Simulation& simulation, const CameraController& camera, UiState& state);

private:
    [[nodiscard]] bool mouseOverScreenPanel(Vector2 mouse, int screenWidth, int screenHeight) const;
};
