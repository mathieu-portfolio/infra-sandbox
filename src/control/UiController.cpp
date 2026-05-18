#include "control/UiController.hpp"

#include "ui/core/UiLayout.hpp"

#include "raylib.h"

#include <array>

namespace {
bool handleLayerToggleClick(const InputEvent& event, UiState& state)
{
    if (event.action != InputAction::Select) {
        return false;
    }

    const UiLayout layout = computeUiLayout(GetScreenWidth(), GetScreenHeight());
    const LeftSidebarLayout left = computeLeftSidebarLayout(layout.leftSidebar, state.sandboxMode);
    const Rectangle layers = left.layers;
    const std::array<UiLayer, 5> layerIds{UiLayer::Flow, UiLayer::Resources, UiLayer::Persistence, UiLayer::Reliability, UiLayer::Geography};
    const std::array<OverlayMode, 5> overlayModes{OverlayMode::Flow, OverlayMode::Utilization, OverlayMode::Queues, OverlayMode::Reliability, OverlayMode::Latency};

    for (int i = 0; i < 5; ++i) {
        const float rowY = layers.y + 42.0f + static_cast<float>(i) * 26.0f;
        const Rectangle row{layers.x + 8.0f, rowY - 4.0f, layers.width - 16.0f, 24.0f};
        if (!CheckCollisionPointRec(event.mousePosition, row)) {
            continue;
        }
        const auto layerIndex = static_cast<std::size_t>(layerIds[static_cast<std::size_t>(i)]);
        state.enabledLayers[layerIndex] = !state.enabledLayers[layerIndex];
        state.activeOverlay = state.enabledLayers[layerIndex] ? overlayModes[static_cast<std::size_t>(i)] : OverlayMode::None;
        return true;
    }
    return false;
}
}

void UiController::handleActions(std::span<const InputEvent> events, UiState& state)
{
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        if (handleLayerToggleClick(event, state)) {
            continue;
        }

        switch (event.action) {
        case InputAction::ToggleDebugUI:
            state.showDebug = !state.showDebug;
            break;
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
