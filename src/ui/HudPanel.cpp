#include "ui/HudPanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"
#include "ui/UiPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>

namespace {
Rectangle scenarioDroplistBounds()
{
    return {206.0f, 11.0f, 270.0f, 32.0f};
}

Rectangle objectivesDroplistBounds(int screenWidth)
{
    const float rightControlsX = static_cast<float>(screenWidth - 220);
    const float x = 830.0f;
    const float width = std::max(220.0f, rightControlsX - x);
    return {x, 11.0f, width, 32.0f};
}

bool pointInDroplistOrMenu(Vector2 point, Rectangle field, Rectangle menu)
{
    return CheckCollisionPointRec(point, field) || CheckCollisionPointRec(point, menu);
}

void drawTopButton(Rectangle bounds, const char* label, bool active)
{
    DrawRectangleRounded(bounds, 0.16f, 6, active ? Color{37, 120, 255, 220} : Color{22, 27, 34, 235});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, active ? Color{89, 196, 255, 230} : Color{70, 86, 104, 120});
    const int textWidth = MeasureText(label, 14);
    DrawText(label, static_cast<int>(bounds.x + bounds.width * 0.5f - static_cast<float>(textWidth) * 0.5f), static_cast<int>(bounds.y + 8.0f), 14, active ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
}

void drawChevron(Rectangle bounds, bool open)
{
    const float centerX = bounds.x + bounds.width - 18.0f;
    const float centerY = bounds.y + bounds.height * 0.5f;
    if (open) {
        DrawTriangle({centerX - 5.0f, centerY + 3.0f}, {centerX + 5.0f, centerY + 3.0f}, {centerX, centerY - 4.0f}, {139, 148, 158, 255});
    } else {
        DrawTriangle({centerX - 5.0f, centerY - 3.0f}, {centerX + 5.0f, centerY - 3.0f}, {centerX, centerY + 4.0f}, {139, 148, 158, 255});
    }
}

void drawDroplistField(Rectangle bounds, const std::string& label, const std::string& value, bool open)
{
    const Color border = open ? Color{89, 196, 255, 230} : Color{70, 86, 104, 130};
    DrawRectangleRounded(bounds, 0.12f, 6, {22, 27, 34, 235});
    DrawRectangleRoundedLines(bounds, 0.12f, 6, border);
    DrawText(label.c_str(), static_cast<int>(bounds.x + 12.0f), static_cast<int>(bounds.y + 5.0f), 11, {89, 196, 255, 255});
    drawTextClipped(value, {bounds.x + 12.0f, bounds.y + 18.0f, bounds.width - 36.0f, 14.0f}, 13, {230, 237, 243, 255});
    drawChevron(bounds, open);
}

void drawMenuShell(Rectangle bounds)
{
    DrawRectangleRounded(bounds, 0.04f, 8, {14, 20, 28, 244});
    DrawRectangleRoundedLines(bounds, 0.04f, 8, {89, 196, 255, 150});
}

void drawDetailRow(const std::string& label, const std::string& value, float x, float y, float width)
{
    DrawText(label.c_str(), static_cast<int>(x), static_cast<int>(y), 12, {139, 148, 158, 255});
    drawTextClipped(value, {x + 86.0f, y - 1.0f, width - 98.0f, 16.0f}, 13, {230, 237, 243, 255});
}

int scenarioIndexForName(const std::string& name)
{
    const auto scenarios = ScenarioRegistry::createAll();
    for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
        if (scenarios[static_cast<std::size_t>(i)].name == name) {
            return i;
        }
    }
    return -1;
}

Rectangle scenarioRowBounds(Rectangle menu, int index)
{
    return {menu.x + 8.0f, menu.y + 120.0f + static_cast<float>(index) * 34.0f, menu.width - 16.0f, 30.0f};
}
}

void HudPanel::update(UiContext& context, const Simulation&, const ScenarioManager& scenarioManager)
{
    if (context.state == nullptr || !context.state->showHud || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle scenarioField = scenarioDroplistBounds();
    const Rectangle objectiveField = objectivesDroplistBounds(context.screenWidth);
    const auto scenarios = ScenarioRegistry::createAll();
    const Rectangle scenarioMenu{scenarioField.x, scenarioField.y + scenarioField.height + 8.0f, 390.0f, 132.0f + static_cast<float>(scenarios.size()) * 34.0f};
    const auto objectiveRows = scenarioManager.definition().objectives.size() + scenarioManager.definition().failureConditions.size();
    const bool hasSideObjectives = scenarioManager.definition().objectives.size() > 1;
    const Rectangle objectiveMenu{objectiveField.x, objectiveField.y + objectiveField.height + 8.0f, std::min(500.0f, objectiveField.width), 96.0f + static_cast<float>(objectiveRows) * 24.0f + (hasSideObjectives ? 18.0f : 0.0f)};

    if (CheckCollisionPointRec(mouse, scenarioField)) {
        context.state->scenarioDroplistOpen = !context.state->scenarioDroplistOpen;
        context.state->objectivesDroplistOpen = false;
        return;
    }
    if (CheckCollisionPointRec(mouse, objectiveField)) {
        context.state->objectivesDroplistOpen = !context.state->objectivesDroplistOpen;
        context.state->scenarioDroplistOpen = false;
        return;
    }
    if (context.state->scenarioDroplistOpen) {
        const int currentIndex = scenarioIndexForName(scenarioManager.staticDefinition().name);
        for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
            if (!CheckCollisionPointRec(mouse, scenarioRowBounds(scenarioMenu, i))) {
                continue;
            }
            if (i != currentIndex) {
                context.state->requestedScenarioIndex = i;
            }
            context.state->scenarioDroplistOpen = false;
            return;
        }
    }

    if (context.state->scenarioDroplistOpen && !pointInDroplistOrMenu(mouse, scenarioField, scenarioMenu)) {
        context.state->scenarioDroplistOpen = false;
    }
    if (context.state->objectivesDroplistOpen && !pointInDroplistOrMenu(mouse, objectiveField, objectiveMenu)) {
        context.state->objectivesDroplistOpen = false;
    }
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

    const Rectangle scenarioBox = scenarioDroplistBounds();
    drawDroplistField(scenarioBox, "Scenario", scenarioManager.definition().name, context.state->scenarioDroplistOpen);

    const int totalSeconds = static_cast<int>(scenarioManager.elapsedSeconds());
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "Time %02d:%02d", totalSeconds / 60, totalSeconds % 60);
    DrawText(buffer, 498, 20, 15, {230, 237, 243, 255});

    drawTopButton(topBarSpeedButton(layout, 0), "1x", simulation.simulationSpeed() == 1.0);
    drawTopButton(topBarSpeedButton(layout, 1), "2x", simulation.simulationSpeed() == 2.0);
    drawTopButton(topBarSpeedButton(layout, 2), "5x", simulation.simulationSpeed() == 5.0);
    drawTopButton(topBarPauseButton(layout), "||", context.paused);
    drawTopButton(topBarPlayButton(layout), ">", !context.paused);

    const Rectangle objectiveBox = objectivesDroplistBounds(context.screenWidth);
    icons.drawIcon("metric.objective", {objectiveBox.x + 10.0f, 15.0f, 20.0f, 20.0f}, {245, 184, 76, 255});
    drawDroplistField({objectiveBox.x + 38.0f, objectiveBox.y, objectiveBox.width - 38.0f, objectiveBox.height}, "Objectives", scenarioManager.objectiveSummary(), context.state->objectivesDroplistOpen);

    icons.drawIcon("topbar.feedback", {static_cast<float>(context.screenWidth - 210), 15.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Feedback", context.screenWidth - 182, 20, 14, {139, 148, 158, 255});
    icons.drawIcon("topbar.help", {static_cast<float>(context.screenWidth - 104), 15.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Help", context.screenWidth - 78, 20, 14, {139, 148, 158, 255});

    if (context.state->scenarioDroplistOpen) {
        const auto scenarios = ScenarioRegistry::createAll();
        const int currentIndex = scenarioIndexForName(scenarioManager.staticDefinition().name);
        const Rectangle menu{scenarioBox.x, scenarioBox.y + scenarioBox.height + 8.0f, 390.0f, 132.0f + static_cast<float>(scenarios.size()) * 34.0f};
        drawMenuShell(menu);
        drawTextClipped(scenarioManager.definition().description, {menu.x + 14.0f, menu.y + 12.0f, menu.width - 28.0f, 18.0f}, 14, {230, 237, 243, 255});
        drawDetailRow("Archetype", scenarioManager.archetypeSummary(), menu.x + 14.0f, menu.y + 42.0f, menu.width - 28.0f);
        drawDetailRow("Tier", scenarioManager.progressionTierSummary(), menu.x + 14.0f, menu.y + 64.0f, menu.width - 28.0f);
        drawDetailRow("Focus", scenarioManager.focusSummary(), menu.x + 14.0f, menu.y + 86.0f, menu.width - 28.0f);
        drawDetailRow("Modifiers", scenarioManager.activeModifiersSummary(), menu.x + 14.0f, menu.y + 108.0f, menu.width - 28.0f);
        for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
            const Rectangle row = scenarioRowBounds(menu, i);
            const bool current = i == currentIndex;
            DrawRectangleRounded(row, 0.12f, 6, current ? Color{37, 120, 255, 170} : Color{22, 27, 34, 150});
            DrawRectangleRoundedLines(row, 0.12f, 6, current ? Color{89, 196, 255, 190} : Color{70, 86, 104, 95});
            drawTextClipped(scenarios[static_cast<std::size_t>(i)].name, {row.x + 10.0f, row.y + 7.0f, row.width - 126.0f, 16.0f}, 13, {230, 237, 243, 255});
            drawTextClipped(progressionTierName(scenarios[static_cast<std::size_t>(i)].minimumTier), {row.x + row.width - 108.0f, row.y + 7.0f, 98.0f, 16.0f}, 12, current ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
        }
    }

    if (context.state->objectivesDroplistOpen) {
        const auto objectiveRows = scenarioManager.definition().objectives.size() + scenarioManager.definition().failureConditions.size();
        const bool hasSideObjectives = scenarioManager.definition().objectives.size() > 1;
        const Rectangle menu{objectiveBox.x, objectiveBox.y + objectiveBox.height + 8.0f, std::min(500.0f, objectiveBox.width), 96.0f + static_cast<float>(objectiveRows) * 24.0f + (hasSideObjectives ? 18.0f : 0.0f)};
        drawMenuShell(menu);
        const float progress = static_cast<float>(std::clamp(scenarioManager.run().objectiveProgress, 0.0, 1.0));
        DrawText("Progress", static_cast<int>(menu.x + 14.0f), static_cast<int>(menu.y + 12.0f), 12, {139, 148, 158, 255});
        DrawRectangleRounded({menu.x + 84.0f, menu.y + 14.0f, menu.width - 104.0f, 8.0f}, 0.5f, 8, {50, 58, 70, 220});
        DrawRectangleRounded({menu.x + 84.0f, menu.y + 14.0f, (menu.width - 104.0f) * progress, 8.0f}, 0.5f, 8, {86, 210, 151, 235});

        float rowY = menu.y + 36.0f;
        if (!scenarioManager.definition().objectives.empty()) {
            drawDetailRow("Primary", scenarioManager.definition().objectives.front().summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += 24.0f;
        }

        if (hasSideObjectives) {
            DrawText("Side objectives", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {89, 196, 255, 255});
            rowY += 20.0f;
            for (std::size_t i = 1; i < scenarioManager.definition().objectives.size(); ++i) {
                drawDetailRow("Optional", scenarioManager.definition().objectives[i].summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
                rowY += 24.0f;
            }
        } else {
            drawDetailRow("Optional", "No side objectives in this scenario.", menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += 24.0f;
        }

        DrawText("Failure limits", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {245, 184, 76, 255});
        rowY += 20.0f;
        for (const auto& failure : scenarioManager.definition().failureConditions) {
            drawDetailRow("Limit", failure.summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += 24.0f;
        }
        if (scenarioManager.definition().failureConditions.empty()) {
            drawDetailRow("Limit", "No failure limits configured.", menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += 24.0f;
        }
        if (const ScenarioPhase* phase = scenarioManager.currentPhase()) {
            drawDetailRow("Phase", phase->name, menu.x + 14.0f, rowY, menu.width - 28.0f);
        }
    }
}
