#include "control/SimulationController.hpp"

#include "ui/UiLayout.hpp"

#include "raylib.h"

SimulationController::Result SimulationController::handleActions(std::span<const InputEvent> events, Simulation& simulation, bool& paused)
{
    Result result;
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        if (event.action == InputAction::Select) {
            const UiLayout layout = computeUiLayout(GetScreenWidth(), GetScreenHeight());
            if (CheckCollisionPointRec(event.mousePosition, topBarPauseButton(layout))) {
                paused = true;
                continue;
            }
            if (CheckCollisionPointRec(event.mousePosition, topBarPlayButton(layout))) {
                paused = false;
                continue;
            }
            for (int i = 0; i < 3; ++i) {
                if (CheckCollisionPointRec(event.mousePosition, topBarSpeedButton(layout, i))) {
                    const double speeds[3] = {1.0, 2.0, 5.0};
                    simulation.setSimulationSpeed(speeds[i]);
                    break;
                }
            }
            continue;
        }

        switch (event.action) {
        case InputAction::PauseSimulation:
            paused = !paused;
            break;
        case InputAction::StepSimulation:
            result.stepRequested = true;
            break;
        case InputAction::ResetSimulation:
            result.resetRequested = true;
            break;
        case InputAction::SetSimulationSpeed1x:
            simulation.setSimulationSpeed(1.0);
            break;
        case InputAction::SetSimulationSpeed2x:
            simulation.setSimulationSpeed(2.0);
            break;
        case InputAction::SetSimulationSpeed5x:
            simulation.setSimulationSpeed(5.0);
            break;
        case InputAction::IncreaseSimulationSpeed:
            simulation.setSimulationSpeed(simulation.simulationSpeed() >= 2.0 ? 5.0 : 2.0);
            break;
        case InputAction::DecreaseSimulationSpeed:
            simulation.setSimulationSpeed(simulation.simulationSpeed() > 2.0 ? 2.0 : 1.0);
            break;
        default:
            break;
        }
    }
    return result;
}
