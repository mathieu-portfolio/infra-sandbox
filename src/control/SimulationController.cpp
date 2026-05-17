#include "control/SimulationController.hpp"

SimulationController::Result SimulationController::handleActions(std::span<const InputEvent> events, Simulation& simulation, bool& paused)
{
    Result result;
    (void)simulation;
    (void)paused;
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        switch (event.action) {
        case InputAction::ResetSimulation:
            result.resetRequested = true;
            break;
        default:
            break;
        }
    }
    return result;
}
