#include "ui/HudPanel.hpp"

#include "raylib.h"

#include <cstdio>

void HudPanel::update(UiContext&, const Simulation&)
{
}

void HudPanel::draw(const UiContext& context, const Simulation&, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr || !context.state->showHud) {
        return;
    }

    char controls[256];
    std::snprintf(
        controls,
        sizeof(controls),
        "Space pause | . step | +/- demand | 1/3/5 speed | IJKL pan | wheel zoom | A scale | 2 cache | T retries | F1-F8 overlays | F11/F12 diag | F9/F10 UI | Overlay: %s",
        overlayModeName(context.state->activeOverlay));

    const int size = 16;
    const int width = MeasureText(controls, size);
    DrawRectangleRounded({12.0f, static_cast<float>(context.screenHeight - 42), static_cast<float>(width + 18), 30.0f}, 0.15f, 8, {22, 27, 34, 210});
    DrawText(controls, 21, context.screenHeight - 35, size, {139, 148, 158, 255});

    const auto* phase = scenarioManager.currentPhase();
    const char* phaseName = phase != nullptr ? phase->name.c_str() : "Free run";
    DrawRectangleRounded({10.0f, 338.0f, 420.0f, 142.0f}, 0.04f, 8, {22, 27, 34, 225});
    DrawText(scenarioManager.definition().name.c_str(), 18, 348, 18, {230, 237, 243, 255});
    DrawText(phaseName, 18, 374, 16, {139, 148, 158, 255});
    DrawText(scenarioManager.objectiveSummary().c_str(), 18, 398, 16, {230, 237, 243, 255});
    DrawText(scenarioManager.focusSummary().c_str(), 18, 424, 16, {139, 148, 158, 255});
    DrawText(scenarioManager.availableMechanicsSummary().c_str(), 18, 450, 16, {139, 148, 158, 255});
}
