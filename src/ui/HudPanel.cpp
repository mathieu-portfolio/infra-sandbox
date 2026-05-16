#include "ui/HudPanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"

#include "raylib.h"

#include <cstdio>

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

    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "Time %.0fs", scenarioManager.elapsedSeconds());
    DrawText(buffer, 512, 20, 15, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Speed %.0fx", simulation.simulationSpeed());
    DrawText(buffer, 610, 20, 15, {230, 237, 243, 255});
    DrawText(context.paused ? "Paused" : "Running", 704, 20, 15, context.paused ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});

    icons.drawIcon("metric.objective", {842.0f, 14.0f, 24.0f, 24.0f}, {245, 184, 76, 255});
    DrawText("Objective", 874, 11, 13, {89, 196, 255, 255});
    DrawText(scenarioManager.objectiveSummary().c_str(), 874, 28, 14, {230, 237, 243, 255});

    icons.drawIcon("topbar.feedback", {static_cast<float>(context.screenWidth - 210), 15.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Feedback", context.screenWidth - 182, 20, 14, {139, 148, 158, 255});
    icons.drawIcon("topbar.help", {static_cast<float>(context.screenWidth - 104), 15.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Help", context.screenWidth - 78, 20, 14, {139, 148, 158, 255});
}
