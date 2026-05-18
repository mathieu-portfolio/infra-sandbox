#include "ui/hud/ScenarioDropdownPanel.hpp"

#include "ui/hud/HudPanelPrimitives.hpp"
#include "ui/UiPrimitives.hpp"

#include <cstddef>

namespace {
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

Rectangle ScenarioDropdownPanel::menuBounds(const UiContext& context) const
{
    const Rectangle field = hudScenarioDroplistBounds(context.screenWidth);
    const auto scenarios = ScenarioRegistry::createAll();
    return {field.x, field.y + field.height + 8.0f, 390.0f, 132.0f + static_cast<float>(scenarios.size()) * 34.0f};
}

bool ScenarioDropdownPanel::update(UiContext& context, const ScenarioManager& scenarioManager, Vector2 mouse)
{
    if (context.state == nullptr) {
        return false;
    }

    const Rectangle field = hudScenarioDroplistBounds(context.screenWidth);
    if (CheckCollisionPointRec(mouse, field)) {
        context.state->scenarioDroplistOpen = !context.state->scenarioDroplistOpen;
        context.state->objectivesDroplistOpen = false;
        context.state->optionsMenuOpen = false;
        return true;
    }

    if (!context.state->scenarioDroplistOpen) {
        return false;
    }

    const Rectangle menu = menuBounds(context);
    const auto scenarios = ScenarioRegistry::createAll();
    const int currentIndex = scenarioIndexForName(scenarioManager.staticDefinition().name);
    for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
        if (!CheckCollisionPointRec(mouse, scenarioRowBounds(menu, i))) {
            continue;
        }
        if (i != currentIndex && scenarioManager.isScenarioUnlocked(scenarios[static_cast<std::size_t>(i)])) {
            context.state->requestedScenarioIndex = i;
        }
        context.state->scenarioDroplistOpen = false;
        return true;
    }

    if (!hudPointInFieldOrMenu(mouse, field, menu)) {
        context.state->scenarioDroplistOpen = false;
    }
    return false;
}

void ScenarioDropdownPanel::drawField(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr) {
        return;
    }
    drawHudDroplistField(hudScenarioDroplistBounds(context.screenWidth), "Scenario", scenarioManager.definition().name, context.state->scenarioDroplistOpen);
}

void ScenarioDropdownPanel::drawMenu(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr || !context.state->scenarioDroplistOpen) {
        return;
    }

    const auto scenarios = ScenarioRegistry::createAll();
    const int currentIndex = scenarioIndexForName(scenarioManager.staticDefinition().name);
    const Rectangle menu = menuBounds(context);
    drawHudMenuShell(menu);
    drawTextClipped(scenarioManager.definition().description, {menu.x + 14.0f, menu.y + 12.0f, menu.width - 28.0f, 18.0f}, 14, {230, 237, 243, 255});
    drawHudDetailRow("Archetype", scenarioManager.archetypeSummary(), menu.x + 14.0f, menu.y + 42.0f, menu.width - 28.0f);
    drawHudDetailRow("Tier", scenarioManager.progressionTierSummary(), menu.x + 14.0f, menu.y + 64.0f, menu.width - 28.0f);
    drawHudDetailRow("Focus", scenarioManager.focusSummary(), menu.x + 14.0f, menu.y + 86.0f, menu.width - 28.0f);
    drawHudDetailRow("Modifiers", scenarioManager.activeModifiersSummary(), menu.x + 14.0f, menu.y + 108.0f, menu.width - 28.0f);
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
