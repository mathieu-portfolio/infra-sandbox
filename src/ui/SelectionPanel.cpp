#include "ui/SelectionPanel.hpp"

#include "raylib.h"

#include <cstdio>

void SelectionPanel::update(UiContext&, const Simulation&)
{
}

void SelectionPanel::draw(const UiContext& context, const Simulation& simulation) const
{
    if (context.state == nullptr || context.state->selection.nodeId < 0) {
        return;
    }

    const Node* node = simulation.graph().node(context.state->selection.nodeId);
    if (node == nullptr) {
        return;
    }

    const auto& definition = NodeRegistry::definition(node->type);
    const int x = context.screenWidth - 312;
    const int y = context.screenHeight - 190;
    DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y), 300.0f, 144.0f}, 0.04f, 8, {22, 27, 34, 235});
    DrawText("Selection", x + 12, y + 10, 18, {230, 237, 243, 255});
    DrawText(node->name.c_str(), x + 12, y + 38, 18, {230, 237, 243, 255});
    DrawText(definition.displayName.data(), x + 12, y + 64, 16, {139, 148, 158, 255});

    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Queue: %zu", node->queue.size());
    DrawText(buffer, x + 12, y + 92, 16, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Utilization: %.0f%%", node->currentUtilization * 100.0);
    DrawText(buffer, x + 12, y + 116, 16, {230, 237, 243, 255});
}
