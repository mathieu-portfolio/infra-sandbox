#include "ui/TimelinePanel.hpp"

#include "raylib.h"

#include <cstdio>

void TimelinePanel::update(UiContext&, const Simulation&)
{
}

void TimelinePanel::draw(const UiContext& context, const Simulation& simulation) const
{
    if (simulation.pressure().recentEvents.empty()) {
        return;
    }

    const int x = context.screenWidth - 312;
    const int y = 264;
    DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y), 300.0f, 124.0f}, 0.04f, 8, {22, 27, 34, 220});
    DrawText("Pressure events", x + 12, y + 10, 18, {230, 237, 243, 255});

    int row = 0;
    for (auto it = simulation.pressure().recentEvents.rbegin(); it != simulation.pressure().recentEvents.rend() && row < 4; ++it) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "%.0fs  %s", it->timeSeconds, it->summary.c_str());
        DrawText(buffer, x + 12, y + 38 + row * 20, 15, {139, 148, 158, 255});
        ++row;
    }
}
