#include "ui/HudPanel.hpp"

#include "raylib.h"

#include <cstdio>

void HudPanel::update(UiContext&, const Simulation&)
{
}

void HudPanel::draw(const UiContext& context, const Simulation&) const
{
    if (context.state == nullptr || !context.state->showHud) {
        return;
    }

    char controls[256];
    std::snprintf(
        controls,
        sizeof(controls),
        "Space pause | . step | +/- demand | 1/3/5 speed | IJKL pan | wheel zoom | A scale | 2 cache | T retries | F1-F8 overlays | F9/F10 UI | Overlay: %s",
        overlayModeName(context.state->activeOverlay));

    const int size = 16;
    const int width = MeasureText(controls, size);
    DrawRectangleRounded({12.0f, static_cast<float>(context.screenHeight - 42), static_cast<float>(width + 18), 30.0f}, 0.15f, 8, {22, 27, 34, 210});
    DrawText(controls, 21, context.screenHeight - 35, size, {139, 148, 158, 255});
}
