#include "ui/DebugPanel.hpp"

#include "raylib.h"

#include <cstdio>

void DebugPanel::update(UiContext&, const Simulation&)
{
}

void DebugPanel::draw(const UiContext& context, const Simulation& simulation) const
{
    if (context.state == nullptr || !context.state->showDebug) {
        return;
    }

    const int panelWidth = 300;
    const int x = context.screenWidth - panelWidth - 12;
    const int y = 12;
    DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y), static_cast<float>(panelWidth), 238.0f}, 0.04f, 8, {22, 27, 34, 235});

    char buffer[128];
    const auto& metrics = simulation.metrics();
    std::snprintf(
        buffer,
        sizeof(buffer),
        "Systems %d/%d  |  %.0fx",
        metrics.observability.enabledSystemCount,
        metrics.observability.initializedSystemCount,
        simulation.simulationSpeed());
    DrawText(buffer, x + 10, y + 10, 18, {230, 237, 243, 255});

    int row = 0;
    for (const auto& state : simulation.runtimeSystems().states()) {
        if (!state.enabled) {
            continue;
        }

        const auto& definition = LayerRegistry::definition(state.layer);
        const Color color{definition.debugColor.r, definition.debugColor.g, definition.debugColor.b, 255};
        const int lineY = y + 42 + row * 22;
        DrawCircleV({static_cast<float>(x + 18), static_cast<float>(lineY + 8)}, 4.0f, color);
        DrawText(state.name, x + 30, lineY, 16, {139, 148, 158, 255});

        ++row;
        if (row >= 5) {
            break;
        }
    }

    const int remaining = metrics.observability.enabledSystemCount - row;
    if (remaining > 0) {
        std::snprintf(buffer, sizeof(buffer), "+ %d more systems", remaining);
        DrawText(buffer, x + 30, y + 42 + row * 22, 16, {139, 148, 158, 255});
    }

    int hintY = y + 186;
    int hintCount = 0;
    for (const auto& hint : simulation.pressure().hints) {
        if (hintCount >= 2) {
            break;
        }
        DrawText(hint.c_str(), x + 10, hintY + hintCount * 20, 14, {245, 184, 76, 255});
        ++hintCount;
    }
}
