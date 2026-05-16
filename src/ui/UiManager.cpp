#include "ui/UiManager.hpp"

#include "rendering/RenderPrimitives.hpp"

#include "raylib.h"

#include <cmath>

void UiManager::update(const Simulation& simulation, bool paused)
{
    overlayController_.update(state_);
    updateSelection(simulation);

    UiContext context{&state_, GetScreenWidth(), GetScreenHeight(), paused};
    hudPanel_.update(context, simulation);
    metricsPanel_.update(context, simulation);
    selectionPanel_.update(context, simulation);
    interventionPanel_.update(context, simulation);
    timelinePanel_.update(context, simulation);
    debugPanel_.update(context, simulation);
}

void UiManager::draw(const Simulation& simulation, bool paused) const
{
    UiState* mutableState = const_cast<UiState*>(&state_);
    UiContext context{mutableState, GetScreenWidth(), GetScreenHeight(), paused};
    hudPanel_.draw(context, simulation);
    metricsPanel_.draw(context, simulation);
    selectionPanel_.draw(context, simulation);
    interventionPanel_.draw(context, simulation);
    timelinePanel_.draw(context, simulation);
    debugPanel_.draw(context, simulation);
}

const UiState& UiManager::state() const
{
    return state_;
}

const OverlayController& UiManager::overlayController() const
{
    return overlayController_;
}

void UiManager::updateSelection(const Simulation& simulation)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || mouseOverScreenPanel()) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    int selectedNodeId = -1;
    float bestDistanceSquared = 3600.0f;
    for (const auto& node : simulation.graph().nodes()) {
        const Vector2 center = worldToScreen(node.position, GetScreenWidth(), GetScreenHeight());
        const float dx = mouse.x - center.x;
        const float dy = mouse.y - center.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < bestDistanceSquared) {
            selectedNodeId = node.id;
            bestDistanceSquared = distanceSquared;
        }
    }

    state_.selection.nodeId = selectedNodeId;
    state_.selection.requestId = -1;
    state_.selection.linkId = -1;
}

bool UiManager::mouseOverScreenPanel() const
{
    const Vector2 mouse = GetMousePosition();
    if (mouse.x <= 380.0f && mouse.y <= 340.0f) {
        return true;
    }
    if (mouse.x >= static_cast<float>(GetScreenWidth() - 324) && mouse.y <= 260.0f) {
        return true;
    }
    if (mouse.y >= static_cast<float>(GetScreenHeight() - 55)) {
        return true;
    }
    return false;
}
