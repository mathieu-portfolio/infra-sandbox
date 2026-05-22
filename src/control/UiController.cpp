#include "control/UiController.hpp"

void UiController::handleActions(std::span<const InputEvent> events, UiState& state)
{
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        switch (event.action) {
        case InputAction::ToggleMetricsUI:
            state.showMetrics = !state.showMetrics;
            break;
        case InputAction::ToggleGeoGrid:
            state.showGeoGrid = !state.showGeoGrid;
            break;
        case InputAction::ClearSelection:
            state.selection = {};
            break;
        default:
            break;
        }
    }
}
