#include "control/OverlayController.hpp"

void OverlayInputController::handleActions(std::span<const InputEvent> events, UiState& state)
{
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        switch (event.action) {
        case InputAction::OverlayFlow:
            state.activeOverlay = OverlayMode::Flow;
            break;
        case InputAction::OverlayLatency:
            state.activeOverlay = OverlayMode::Latency;
            break;
        case InputAction::OverlayUtilization:
            state.activeOverlay = OverlayMode::Utilization;
            break;
        case InputAction::OverlayQueues:
            state.activeOverlay = OverlayMode::Queues;
            break;
        case InputAction::OverlayErrors:
            state.activeOverlay = OverlayMode::Errors;
            break;
        case InputAction::OverlayReliability:
            state.activeOverlay = OverlayMode::Reliability;
            break;
        case InputAction::OverlayComplexity:
            state.activeOverlay = OverlayMode::Complexity;
            break;
        case InputAction::OverlayNone:
            state.activeOverlay = OverlayMode::None;
            break;
        default:
            break;
        }
    }
}
