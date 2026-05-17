#include "ui/HudPanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"
#include "ui/UiPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>

namespace {
void drawTopButton(Rectangle bounds, const char* label, bool active)
{
    DrawRectangleRounded(bounds, 0.16f, 6, active ? Color{37, 120, 255, 220} : Color{22, 27, 34, 235});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, active ? Color{89, 196, 255, 230} : Color{70, 86, 104, 120});
    const int textWidth = MeasureText(label, 14);
    DrawText(label, static_cast<int>(bounds.x + bounds.width * 0.5f - static_cast<float>(textWidth) * 0.5f), static_cast<int>(bounds.y + 8.0f), 14, active ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
}
}

void HudPanel::update(UiContext&, const Simulation&)
{
}

void HudPanel::draw(const UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr || !context.state->showHud) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    DrawRectangleRec(layout.topBar, {8, 13, 20, 246});
    DrawLineEx({0.0f, layout.topBar.height}, {static_cast<float>(context.screenWidth), layout.topBar.height}, 1.0f, {70, 86, 104, 110});

    auto& icons = IconRegistry::instance();
    icons.drawIcon("topbar.logo", {18.0f, 14.0f, 26.0f, 26.0f}, {230, 237, 243, 255});
    DrawText("INFRA SANDBOX", 52, 18, 18, {230, 237, 243, 255});

    const Rectangle scenarioBox{206.0f, 11.0f, 270.0f, 32.0f};
    DrawRectangleRounded(scenarioBox, 0.12f, 6, {22, 27, 34, 235});
    DrawText("Scenario:", 220, 20, 14, {139, 148, 158, 255});
    DrawText(scenarioManager.definition().name.c_str(), 292, 20, 14, {230, 237, 243, 255});

    const int totalSeconds = static_cast<int>(scenarioManager.elapsedSeconds());
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "Time %02d:%02d", totalSeconds / 60, totalSeconds % 60);
    DrawText(buffer, 498, 20, 15, {230, 237, 243, 255});

    drawTopButton(topBarSpeedButton(layout, 0), "1x", simulation.simulationSpeed() == 1.0);
    drawTopButton(topBarSpeedButton(layout, 1), "2x", simulation.simulationSpeed() == 2.0);
    drawTopButton(topBarSpeedButton(layout, 2), "5x", simulation.simulationSpeed() == 5.0);
    drawTopButton(topBarPauseButton(layout), "||", context.paused);
    drawTopButton(topBarPlayButton(layout), ">", !context.paused);

    icons.drawIcon("metric.objective", {790.0f, 14.0f, 24.0f, 24.0f}, {245, 184, 76, 255});
    DrawText("Objective", 822, 11, 13, {89, 196, 255, 255});
    drawTextClipped(scenarioManager.objectiveSummary(), {822.0f, 28.0f, std::max(160.0f, static_cast<float>(context.screenWidth - 1040)), 18.0f}, 14, {230, 237, 243, 255});

    icons.drawIcon("topbar.feedback", {static_cast<float>(context.screenWidth - 210), 15.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Feedback", context.screenWidth - 182, 20, 14, {139, 148, 158, 255});
    icons.drawIcon("topbar.help", {static_cast<float>(context.screenWidth - 104), 15.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Help", context.screenWidth - 78, 20, 14, {139, 148, 158, 255});
}
