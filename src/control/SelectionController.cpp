#include "control/SelectionController.hpp"

#include "rendering/GeoLayoutSystem.hpp"
#include "rendering/RenderPrimitives.hpp"
#include "ui/core/UiLayout.hpp"

#include "raylib.h"

namespace {
Rectangle worldActionToggleBounds(int screenWidth)
{
    return {static_cast<float>(screenWidth) * 0.5f - 120.0f, 68.0f, 240.0f, 34.0f};
}
}

void SelectionController::handleActions(std::span<const InputEvent> events, const Simulation& simulation, const CameraController& camera, UiState& state)
{
    for (const auto& event : events) {
        if (event.action != InputAction::Select || event.phase != InputPhase::Pressed) {
            continue;
        }

        if (state.suppressMapSelectionOnce) {
            state.suppressMapSelectionOnce = false;
            continue;
        }

        if (mouseOverScreenPanel(event.mousePosition, GetScreenWidth(), GetScreenHeight())) {
            continue;
        }
        if (state.gameplayPhase == GameplayPhase::Planning && !state.worldActionDraft.empty()) {
            if (state.worldActionDraftVisible || CheckCollisionPointRec(event.mousePosition, worldActionToggleBounds(GetScreenWidth()))) {
                continue;
            }
        }

        UiState selectionState = state;
        selectionState.selection.nodeId = -1;
        const GeoLayoutFrame layout = GeoLayoutSystem{}.compute(simulation, camera, selectionState, GetScreenWidth(), GetScreenHeight());

        int selectedNodeId = -1;
        float bestDistanceSquared = 3600.0f;
        for (const auto& nodeLayout : layout.nodes) {
            if (nodeLayout.hiddenByCluster) {
                continue;
            }
            const Vector2 center = worldToScreen(nodeLayout.displayPosition, GetScreenWidth(), GetScreenHeight(), camera);
            const float dx = event.mousePosition.x - center.x;
            const float dy = event.mousePosition.y - center.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (distanceSquared < bestDistanceSquared) {
                selectedNodeId = nodeLayout.nodeId;
                bestDistanceSquared = distanceSquared;
            }
        }

        state.selection.nodeId = selectedNodeId;
        state.selection.requestId = -1;
        state.selection.linkId = -1;
        state.selectedActionIndex = -1;
        state.hoveredActionIndex = -1;
    }
}

bool SelectionController::mouseOverScreenPanel(Vector2 mouse, int screenWidth, int screenHeight) const
{
    return pointInUiPanel(mouse, computeUiLayout(screenWidth, screenHeight));
}
