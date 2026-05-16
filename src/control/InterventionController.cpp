#include "control/InterventionController.hpp"

void InterventionController::handleActions(std::span<const InputEvent> events, Simulation& simulation)
{
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        switch (event.action) {
        case InputAction::ScaleUp:
            mechanicExecutor_.execute(simulation, {MechanicType::ScaleUp, -1, 1.5});
            break;
        case InputAction::ScaleOut:
            mechanicExecutor_.execute(simulation, {MechanicType::ScaleOut});
            break;
        case InputAction::ToggleCache:
            mechanicExecutor_.execute(simulation, {MechanicType::EnableCache});
            break;
        case InputAction::ClearCache:
            mechanicExecutor_.execute(simulation, {MechanicType::ClearCache});
            break;
        case InputAction::ToggleRetries:
            mechanicExecutor_.execute(simulation, {MechanicType::ToggleRetries});
            break;
        case InputAction::ToggleTrafficBurst:
            simulation.toggleBurstMode();
            break;
        case InputAction::ResetInterventions:
            simulation.resetProcessingCapacity();
            break;
        case InputAction::EnableTracing:
            mechanicExecutor_.execute(simulation, {MechanicType::EnableTracing});
            break;
        case InputAction::ThrottleTrafficUp:
            mechanicExecutor_.execute(simulation, {MechanicType::ThrottleTraffic, -1, 1.0});
            break;
        case InputAction::ThrottleTrafficDown:
            mechanicExecutor_.execute(simulation, {MechanicType::ThrottleTraffic, -1, -1.0});
            break;
        default:
            break;
        }
    }
}
