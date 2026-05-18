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
TopBarLayout topBarLayoutForWidth(int screenWidth)
{
    return computeTopBarLayout({0.0f, 0.0f, static_cast<float>(screenWidth), UiTheme::topBarHeight});
}

Rectangle scenarioDroplistBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).scenario;
}

Rectangle objectivesDroplistBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).objectives;
}

Rectangle phaseButtonBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).phase;
}

Rectangle feedbackBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).feedback;
}

Rectangle helpBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).help;
}

Rectangle optionsButtonBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).options;
}

Rectangle optionsMenuBounds(int screenWidth, int screenHeight)
{
    constexpr float width = 300.0f;
    constexpr float height = 176.0f;
    return {
        static_cast<float>(screenWidth) * 0.5f - width * 0.5f,
        static_cast<float>(screenHeight) * 0.5f - height * 0.5f,
        width,
        height,
    };
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

void drawIconButton(Rectangle bounds, const char* iconId, bool active)
{
    DrawRectangleRounded(bounds, 0.16f, 6, active ? Color{37, 120, 255, 180} : Color{22, 27, 34, 200});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, active ? Color{89, 196, 255, 210} : Color{70, 86, 104, 95});
    IconRegistry::instance().drawIcon(iconId, {bounds.x + 7.0f, bounds.y + 7.0f, bounds.width - 14.0f, bounds.height - 14.0f}, active ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
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

Rectangle optionsRowBounds(Rectangle menu, int index)
{
    return {menu.x + 10.0f, menu.y + 34.0f + static_cast<float>(index) * 29.0f, menu.width - 20.0f, 25.0f};
}

const char* phaseName(GameplayPhase phase)
{
    switch (phase) {
    case GameplayPhase::Observation:
        return "Observation";
    case GameplayPhase::Planning:
        return "Planning";
    case GameplayPhase::Transition:
        return "Simulating";
    case GameplayPhase::Resolution:
        return "Resolution";
    }
    return "Unknown";
}

const char* phaseActionLabel(GameplayPhase phase)
{
    switch (phase) {
    case GameplayPhase::Observation:
        return "Start Planning";
    case GameplayPhase::Planning:
        return "Validate Plan";
    case GameplayPhase::Transition:
        return "Simulating";
    case GameplayPhase::Resolution:
        return "Analyze / Continue";
    }
    return "Advance";
}

void drawOptionRow(Rectangle row, const char* label, bool enabled)
{
    DrawRectangleRounded(row, 0.12f, 6, {22, 27, 34, 175});
    DrawRectangleRoundedLines(row, 0.12f, 6, {70, 86, 104, 95});
    DrawText(label, static_cast<int>(row.x + 10.0f), static_cast<int>(row.y + 6.0f), 12, {230, 237, 243, 255});
    drawToggle({row.x + row.width - 44.0f, row.y + 4.0f, 34.0f, 17.0f}, enabled);
}
}

void HudPanel::update(UiContext& context, const Simulation&, const ScenarioManager& scenarioManager)
{
    if (context.state == nullptr || !context.state->showHud || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle scenarioField = scenarioDroplistBounds(context.screenWidth);
    const Rectangle objectiveField = objectivesDroplistBounds(context.screenWidth);
    const Rectangle optionsButton = optionsButtonBounds(context.screenWidth);
    const Rectangle optionsMenu = optionsMenuBounds(context.screenWidth, context.screenHeight);
    const Rectangle phaseButton = phaseButtonBounds(context.screenWidth);
    const auto scenarios = ScenarioRegistry::createAll();
    const Rectangle scenarioMenu{scenarioField.x, scenarioField.y + scenarioField.height + 8.0f, 390.0f, 132.0f + static_cast<float>(scenarios.size()) * 34.0f};
    const auto& run = scenarioManager.run();
    const auto objectiveRows = run.activeObjectiveIds.size() + run.completedObjectiveIds.size() + scenarioManager.definition().failureConditions.size() + run.unlockedInterventions.size() + run.unlockedScenarioIds.size();
    const Rectangle objectiveMenu{objectiveField.x, objectiveField.y + objectiveField.height + 8.0f, std::min(540.0f, objectiveField.width), 126.0f + static_cast<float>(objectiveRows) * 21.0f};

    if (CheckCollisionPointRec(mouse, scenarioField)) {
        context.state->scenarioDroplistOpen = !context.state->scenarioDroplistOpen;
        context.state->objectivesDroplistOpen = false;
        context.state->optionsMenuOpen = false;
        return;
    }
    if (CheckCollisionPointRec(mouse, objectiveField)) {
        context.state->objectivesDroplistOpen = !context.state->objectivesDroplistOpen;
        context.state->scenarioDroplistOpen = false;
        context.state->optionsMenuOpen = false;
        return;
    }
    if (CheckCollisionPointRec(mouse, optionsButton)) {
        context.state->optionsMenuOpen = !context.state->optionsMenuOpen;
        context.state->scenarioDroplistOpen = false;
        context.state->objectivesDroplistOpen = false;
        return;
    }
    if (CheckCollisionPointRec(mouse, phaseButton) && context.state->gameplayPhase != GameplayPhase::Transition) {
        if (context.state->gameplayPhase == GameplayPhase::Planning && !context.state->worldActionDraft.empty() && context.state->selectedWorldActionIndex < 0) {
            context.state->worldActionDraftVisible = true;
            context.state->latestFeedback = "Pick a World Action before validating the plan.";
            return;
        }
        context.state->phaseAdvanceRequested = true;
        context.state->scenarioDroplistOpen = false;
        context.state->objectivesDroplistOpen = false;
        context.state->optionsMenuOpen = false;
        return;
    }
    if (context.state->optionsMenuOpen) {
        for (int i = 0; i < 4; ++i) {
            if (!CheckCollisionPointRec(mouse, optionsRowBounds(optionsMenu, i))) {
                continue;
            }
            if (i == 0) {
                context.state->fullscreenToggleRequested = true;
            } else if (i == 1) {
                context.state->showMetrics = !context.state->showMetrics;
            } else if (i == 2) {
                context.state->showDebug = !context.state->showDebug;
            } else if (i == 3) {
                context.state->showGeoGrid = !context.state->showGeoGrid;
            }
            return;
        }
    }
    if (context.state->scenarioDroplistOpen) {
        const int currentIndex = scenarioIndexForName(scenarioManager.staticDefinition().name);
        for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
            if (!CheckCollisionPointRec(mouse, scenarioRowBounds(scenarioMenu, i))) {
                continue;
            }
            if (i != currentIndex && scenarioManager.isScenarioUnlocked(scenarios[static_cast<std::size_t>(i)])) {
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
    if (context.state->optionsMenuOpen && !pointInDroplistOrMenu(mouse, optionsButton, optionsMenu)) {
        context.state->optionsMenuOpen = false;
    }
}

void HudPanel::draw(const UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr || !context.state->showHud) {
        return;
    }
    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const TopBarLayout top = computeTopBarLayout(layout.topBar);
    DrawRectangleRec(layout.topBar, {8, 13, 20, 246});
    DrawLineEx({0.0f, layout.topBar.height}, {static_cast<float>(context.screenWidth), layout.topBar.height}, 1.0f, {70, 86, 104, 110});

    auto& icons = IconRegistry::instance();
    icons.drawIcon("topbar.logo", {top.brand.x + 4.0f, top.brand.y + 4.0f, 26.0f, 26.0f}, {230, 237, 243, 255});
    DrawText("INFRA SANDBOX", static_cast<int>(top.brand.x + 38.0f), static_cast<int>(top.brand.y + 8.0f), 18, {230, 237, 243, 255});

    const Rectangle scenarioBox = scenarioDroplistBounds(context.screenWidth);
    drawDroplistField(scenarioBox, "Scenario", scenarioManager.definition().name, context.state->scenarioDroplistOpen);

    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "%s", scenarioManager.visibleCalendarLabel(simulation).c_str());
    DrawText(buffer, static_cast<int>(top.time.x), static_cast<int>(top.time.y + 10.0f), 15, {230, 237, 243, 255});

    const Rectangle phaseButton = phaseButtonBounds(context.screenWidth);
    drawTopButton(phaseButton, phaseActionLabel(context.state->gameplayPhase), context.state->gameplayPhase == GameplayPhase::Planning);
    DrawText(phaseName(context.state->gameplayPhase), static_cast<int>(top.phaseLabel.x), static_cast<int>(top.phaseLabel.y + 10.0f), 14, {139, 148, 158, 255});

    const Rectangle objectiveBox = objectivesDroplistBounds(context.screenWidth);
    icons.drawIcon("metric.objective", {objectiveBox.x + 2.0f, objectiveBox.y + 4.0f, 20.0f, 20.0f}, {245, 184, 76, 255});
    drawDroplistField({objectiveBox.x + 30.0f, objectiveBox.y, objectiveBox.width - 30.0f, objectiveBox.height}, "Objectives", scenarioManager.objectiveSummary(), context.state->objectivesDroplistOpen);

    const Rectangle feedback = feedbackBounds(context.screenWidth);
    icons.drawIcon("topbar.feedback", {feedback.x, feedback.y + 4.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Feedback", static_cast<int>(feedback.x + 28.0f), static_cast<int>(feedback.y + 10.0f), 14, {139, 148, 158, 255});
    const Rectangle help = helpBounds(context.screenWidth);
    icons.drawIcon("topbar.help", {help.x, help.y + 4.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Help", static_cast<int>(help.x + 26.0f), static_cast<int>(help.y + 10.0f), 14, {139, 148, 158, 255});
    drawIconButton(optionsButtonBounds(context.screenWidth), "topbar.options", context.state->optionsMenuOpen);

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
            const bool unlocked = scenarioManager.isScenarioUnlocked(scenarios[static_cast<std::size_t>(i)]);
            DrawRectangleRounded(row, 0.12f, 6, current ? Color{37, 120, 255, 170} : Color{22, 27, 34, 150});
            DrawRectangleRoundedLines(row, 0.12f, 6, current ? Color{89, 196, 255, 190} : Color{70, 86, 104, 95});
            drawTextClipped(scenarios[static_cast<std::size_t>(i)].name, {row.x + 10.0f, row.y + 7.0f, row.width - 126.0f, 16.0f}, 13, unlocked ? Color{230, 237, 243, 255} : Color{90, 107, 126, 255});
            drawTextClipped(unlocked ? progressionTierName(scenarios[static_cast<std::size_t>(i)].minimumTier) : "Locked", {row.x + row.width - 108.0f, row.y + 7.0f, 98.0f, 16.0f}, 12, current ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
        }
    }

    if (context.state->objectivesDroplistOpen) {
        const auto& run = scenarioManager.run();
        const auto objectiveRows = run.activeObjectiveIds.size() + run.completedObjectiveIds.size() + scenarioManager.definition().failureConditions.size() + run.unlockedInterventions.size() + run.unlockedScenarioIds.size();
        const Rectangle menu{objectiveBox.x, objectiveBox.y + objectiveBox.height + 8.0f, std::min(540.0f, objectiveBox.width), 126.0f + static_cast<float>(objectiveRows) * 21.0f};
        drawMenuShell(menu);
        const float progress = static_cast<float>(std::clamp(scenarioManager.run().objectiveProgress, 0.0, 1.0));
        DrawText("Progress", static_cast<int>(menu.x + 14.0f), static_cast<int>(menu.y + 12.0f), 12, {139, 148, 158, 255});
        DrawRectangleRounded({menu.x + 84.0f, menu.y + 14.0f, menu.width - 104.0f, 8.0f}, 0.5f, 8, {50, 58, 70, 220});
        DrawRectangleRounded({menu.x + 84.0f, menu.y + 14.0f, (menu.width - 104.0f) * progress, 8.0f}, 0.5f, 8, {86, 210, 151, 235});

        float rowY = menu.y + 36.0f;
        DrawText("Active objectives", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {89, 196, 255, 255});
        rowY += 20.0f;
        for (const auto& id : run.activeObjectiveIds) {
            const auto it = std::find_if(scenarioManager.definition().objectives.begin(), scenarioManager.definition().objectives.end(), [&id](const ScenarioObjective& objective) { return objective.id == id; });
            if (it != scenarioManager.definition().objectives.end()) {
                drawDetailRow("Active", it->summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
                rowY += 21.0f;
            }
        }
        if (!run.completedObjectiveIds.empty()) {
            DrawText("Completed", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {86, 210, 151, 255});
            rowY += 20.0f;
            for (const auto& id : run.completedObjectiveIds) {
                const auto it = std::find_if(scenarioManager.definition().objectives.begin(), scenarioManager.definition().objectives.end(), [&id](const ScenarioObjective& objective) { return objective.id == id; });
                if (it != scenarioManager.definition().objectives.end()) {
                    drawDetailRow("Done", it->summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
                    rowY += 21.0f;
                }
            }
        }

        DrawText("Failure limits", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {245, 184, 76, 255});
        rowY += 20.0f;
        for (const auto& failure : scenarioManager.definition().failureConditions) {
            drawDetailRow("Limit", failure.summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += 21.0f;
        }
        if (scenarioManager.definition().failureConditions.empty()) {
            drawDetailRow("Limit", "No failure limits configured.", menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += 21.0f;
        }
        DrawText("Unlocked actions", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {86, 210, 151, 255});
        rowY += 20.0f;
        if (run.unlockedInterventions.empty()) {
            drawDetailRow("Action", "None yet.", menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += 21.0f;
        }
        for (const auto mechanic : run.unlockedInterventions) {
            drawDetailRow("Action", std::string(MechanicRegistry::definition(mechanic).displayName), menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += 21.0f;
        }
        if (!run.unlockedScenarioIds.empty()) {
            DrawText("Unlocked scenarios", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {151, 111, 255, 255});
            rowY += 20.0f;
            for (const auto& id : run.unlockedScenarioIds) {
                drawDetailRow("Scenario", id, menu.x + 14.0f, rowY, menu.width - 28.0f);
                rowY += 21.0f;
            }
        }
        if (const ScenarioPhase* phase = scenarioManager.currentPhase()) {
            drawDetailRow("Phase", phase->name, menu.x + 14.0f, rowY, menu.width - 28.0f);
        }
    }

    if (context.state->optionsMenuOpen) {
        const Rectangle menu = optionsMenuBounds(context.screenWidth, context.screenHeight);
        drawMenuShell(menu);
        DrawText("Options", static_cast<int>(menu.x + 12.0f), static_cast<int>(menu.y + 12.0f), 14, {89, 196, 255, 255});
        drawOptionRow(optionsRowBounds(menu, 0), "Maximized", IsWindowMaximized());
        drawOptionRow(optionsRowBounds(menu, 1), "Metrics panel", context.state->showMetrics);
        drawOptionRow(optionsRowBounds(menu, 2), "Debug UI", context.state->showDebug);
        drawOptionRow(optionsRowBounds(menu, 3), "Map grid", context.state->showGeoGrid);
    }
}
