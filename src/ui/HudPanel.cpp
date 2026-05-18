#include "ui/HudPanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/hud/HudPanelPrimitives.hpp"
#include "ui/core/UiLayout.hpp"

#include "raylib.h"

#include <cstdio>

namespace {
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

bool handlePhaseButton(UiContext& context, Vector2 mouse)
{
    if (context.state == nullptr) {
        return false;
    }

    if (!CheckCollisionPointRec(mouse, hudPhaseButtonBounds(context.screenWidth)) || context.state->gameplayPhase == GameplayPhase::Transition) {
        return false;
    }

    if (context.state->gameplayPhase == GameplayPhase::Planning && !context.state->worldActionDraft.empty() && context.state->selectedWorldActionIndex < 0) {
        context.state->worldActionDraftVisible = true;
        context.state->latestFeedback = "Pick a World Action before validating the plan.";
        return true;
    }

    context.state->phaseAdvanceRequested = true;
    context.state->scenarioDroplistOpen = false;
    context.state->objectivesDroplistOpen = false;
    context.state->optionsMenuOpen = false;
    return true;
}
}

void HudPanel::update(UiContext& context, const Simulation&, const ScenarioManager& scenarioManager)
{
    if (context.state == nullptr || !context.state->showHud || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    if (scenarioDropdown_.update(context, scenarioManager, mouse)) {
        return;
    }
    if (objectivesDropdown_.update(context, scenarioManager, mouse)) {
        return;
    }
    if (optionsMenu_.update(context, mouse)) {
        return;
    }
    handlePhaseButton(context, mouse);
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

    scenarioDropdown_.drawField(context, scenarioManager);

    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "%s", scenarioManager.visibleCalendarLabel(simulation).c_str());
    DrawText(buffer, static_cast<int>(top.time.x), static_cast<int>(top.time.y + 10.0f), 15, {230, 237, 243, 255});

    drawHudTopButton(hudPhaseButtonBounds(context.screenWidth), phaseActionLabel(context.state->gameplayPhase), context.state->gameplayPhase == GameplayPhase::Planning);
    DrawText(phaseName(context.state->gameplayPhase), static_cast<int>(top.phaseLabel.x), static_cast<int>(top.phaseLabel.y + 10.0f), 14, {139, 148, 158, 255});

    objectivesDropdown_.drawField(context, scenarioManager);

    const Rectangle feedback = hudFeedbackBounds(context.screenWidth);
    icons.drawIcon("topbar.feedback", {feedback.x, feedback.y + 4.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Feedback", static_cast<int>(feedback.x + 28.0f), static_cast<int>(feedback.y + 10.0f), 14, {139, 148, 158, 255});

    const Rectangle help = hudHelpBounds(context.screenWidth);
    icons.drawIcon("topbar.help", {help.x, help.y + 4.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("Help", static_cast<int>(help.x + 26.0f), static_cast<int>(help.y + 10.0f), 14, {139, 148, 158, 255});

    optionsMenu_.drawButton(context);

    scenarioDropdown_.drawMenu(context, scenarioManager);
    objectivesDropdown_.drawMenu(context, scenarioManager);
    optionsMenu_.drawMenu(context);
}
