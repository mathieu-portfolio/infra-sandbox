#include "ui/panels/HudPanel.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/hud/HudPanelPrimitives.hpp"
#include "ui/actions/EventOverlay.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"

#include "raylib.h"

#include <cstdio>
#include <array>

namespace {
const char* phaseName(GameplayPhase phase)
{
    switch (phase) {
    case GameplayPhase::Planning:
        return "Planning";
    case GameplayPhase::Resolving:
        return "Resolving";
    case GameplayPhase::Analysis:
        return "Analysis";
    }
    return "Unknown";
}

const char* phaseActionLabel(GameplayPhase phase)
{
    switch (phase) {
    case GameplayPhase::Planning:
        return "Resolve Turn";
    case GameplayPhase::Resolving:
        return "Resolving";
    case GameplayPhase::Analysis:
        return "Next Planning";
    }
    return "Advance";
}


constexpr int kViewModeCount = static_cast<int>(UiViewMode::Count);

UiViewMode viewModeAt(int index)
{
    return static_cast<UiViewMode>(index);
}

void selectViewMode(UiState& state, UiViewMode mode)
{
    state.activeViewMode = mode;
    state.activeOverlay = overlayForViewMode(mode);
    state.scenarioDroplistOpen = false;
    state.packDroplistOpen = false;
    state.objectivesDroplistOpen = false;
    state.optionsMenuOpen = false;
}

bool handleViewModeBar(UiContext& context, Vector2 mouse)
{
    if (context.state == nullptr) {
        return false;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight, context.state->dockLayout);
    const Rectangle bar = computeViewModeBarBounds(layout.worldView);
    if (!CheckCollisionPointRec(mouse, bar)) {
        return false;
    }

    for (int i = 0; i < kViewModeCount; ++i) {
        if (CheckCollisionPointRec(mouse, computeViewModeButtonBounds(bar, i, kViewModeCount))) {
            selectViewMode(*context.state, viewModeAt(i));
            return true;
        }
    }

    return true;
}

bool handleViewModeShortcuts(UiContext& context)
{
    if (context.state == nullptr) {
        return false;
    }

    const std::array<int, kViewModeCount> keys{KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX};
    for (int i = 0; i < kViewModeCount; ++i) {
        if (IsKeyPressed(keys[static_cast<std::size_t>(i)])) {
            selectViewMode(*context.state, viewModeAt(i));
            return true;
        }
    }
    return false;
}

void drawViewModeBar(const UiContext& context)
{
    if (context.state == nullptr) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight, context.state->dockLayout);
    const Rectangle bar = computeViewModeBarBounds(layout.worldView);
    DrawRectangleRounded(bar, 0.28f, 12, {12, 18, 27, 218});
    DrawRectangleRoundedLines(bar, 0.28f, 12, {70, 86, 104, 105});

    for (int i = 0; i < kViewModeCount; ++i) {
        const UiViewMode mode = viewModeAt(i);
        const Rectangle button = computeViewModeButtonBounds(bar, i, kViewModeCount);
        const bool active = context.state->activeViewMode == mode;
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), button);
        const Color fill = active ? Color{72, 52, 164, 238} : hovered ? Color{30, 38, 50, 230} : Color{18, 24, 34, 185};
        const Color stroke = active ? Color{146, 104, 255, 180} : Color{70, 86, 104, 85};
        const Color text = active ? Color{240, 236, 255, 255} : Color{169, 179, 190, 255};

        DrawRectangleRounded(button, 0.24f, 8, fill);
        DrawRectangleRoundedLines(button, 0.24f, 8, stroke);
        drawIconLabelRow({button.x + 9.0f, button.y + 4.0f, button.width - 18.0f, button.height - 8.0f}, uiViewModeIcon(mode), uiViewModeName(mode), {
            14.0f,
            5.0f,
            13,
            text,
            text,
        });
    }
}

bool handlePhaseButton(UiContext& context, Vector2 mouse)
{
    if (context.state == nullptr) {
        return false;
    }

    if (!CheckCollisionPointRec(mouse, hudPhaseButtonBounds(context.screenWidth)) || context.state->gameplayPhase == GameplayPhase::Resolving) {
        return false;
    }

    if (context.state->eventPopupMode != EventPopupMode::None) {
        context.state->latestFeedback = context.state->eventPopupMode == EventPopupMode::PlanningStart
            ? "Review the event briefing before validating the plan."
            : "Review the event recap before continuing.";
        return true;
    }

    if (context.state->gameplayPhase == GameplayPhase::Planning && !context.state->worldActionDraft.empty() && context.state->selectedWorldActionIndex < 0) {
        context.state->worldActionDraftVisible = true;
        context.state->latestFeedback = "Pick a World Action before resolving the turn.";
        return true;
    }

    context.state->phaseAdvanceRequested = true;
    context.state->packDroplistOpen = false;
    context.state->scenarioDroplistOpen = false;
    context.state->objectivesDroplistOpen = false;
    context.state->optionsMenuOpen = false;
    return true;
}
}

void HudPanel::update(UiContext& context, const UiFrameView&, const UiScenarioView& scenarioView, const content::ContentPackManager& packManager)
{
    if (context.state == nullptr || !context.state->showHud) {
        return;
    }

    handleViewModeShortcuts(context);

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    if (handleViewModeBar(context, mouse)) {
        return;
    }
    if (packDropdown_.update(context, packManager, mouse)) {
        return;
    }
    if (scenarioDropdown_.update(context, scenarioView, mouse)) {
        return;
    }
    if (objectivesDropdown_.update(context, scenarioView, mouse)) {
        return;
    }
    if (context.state->eventPopupMode != EventPopupMode::None) {
        const EventPopupMode mode = context.state->eventPopupMode;
        const Rectangle button = EventOverlay::acknowledgeButtonBounds(
            context.screenWidth,
            context.screenHeight,
            mode,
            context.state->eventPopupEvents);
        if (CheckCollisionPointRec(mouse, button)) {
            context.state->eventPopupMode = EventPopupMode::None;
            context.state->eventPanelAcknowledged = true;
            context.state->latestFeedback.clear();
            if (mode == EventPopupMode::PlanningStart && !context.state->worldActionDraft.empty()) {
                context.state->worldActionDraftVisible = true;
            }
        }
        return;
    }
    if (CheckCollisionPointRec(mouse, hudResetButtonBounds(context.screenWidth))) {
        context.state->resetScenarioRequested = true;
        context.state->latestFeedback = "Resetting scenario.";
        return;
    }
    if (optionsMenu_.update(context, mouse)) {
        return;
    }
    handlePhaseButton(context, mouse);
}

void HudPanel::draw(const UiContext& context, const UiFrameView& view, const UiScenarioView& scenarioView, const content::ContentPackManager& packManager) const
{
    if (context.state == nullptr || !context.state->showHud) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight, context.state->dockLayout);
    const TopBarLayout top = computeTopBarLayout(layout.topBar);
    DrawRectangleRec(layout.topBar, {8, 13, 20, 246});
    DrawLineEx({0.0f, layout.topBar.height}, {static_cast<float>(context.screenWidth), layout.topBar.height}, 1.0f, {70, 86, 104, 110});

    drawIconLabelRow({top.brand.x + 4.0f, top.brand.y, top.brand.width - 4.0f, top.brand.height}, "hud.logo", "INFRA SANDBOX", {
        26.0f,
        8.0f,
        18,
        {230, 237, 243, 255},
        {230, 237, 243, 255},
    });

    packDropdown_.drawField(context, packManager);
    scenarioDropdown_.drawField(context, scenarioView);

    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "%s", scenarioView.visibleCalendarLabel().c_str());
    DrawText(buffer, static_cast<int>(top.time.x), static_cast<int>(top.time.y + 10.0f), 15, {230, 237, 243, 255});

    drawHudTopButton(hudPhaseButtonBounds(context.screenWidth), phaseActionLabel(context.state->gameplayPhase), context.state->gameplayPhase == GameplayPhase::Planning);
    drawHudTopButton(hudResetButtonBounds(context.screenWidth), "Reset", false);
    DrawText(phaseName(context.state->gameplayPhase), static_cast<int>(top.phaseLabel.x), static_cast<int>(top.phaseLabel.y + 10.0f), 14, {139, 148, 158, 255});

    objectivesDropdown_.drawField(context, scenarioView);

    const Rectangle feedback = hudFeedbackBounds(context.screenWidth);
    drawIconLabelRow({feedback.x, feedback.y, feedback.width, feedback.height}, "hud.feedback", "Feedback", {
        22.0f,
        6.0f,
        14,
        {139, 148, 158, 255},
        {139, 148, 158, 255},
    });

    const Rectangle help = hudHelpBounds(context.screenWidth);
    drawIconLabelRow({help.x, help.y, help.width, help.height}, "hud.help", "Help", {
        22.0f,
        4.0f,
        14,
        {139, 148, 158, 255},
        {139, 148, 158, 255},
    });

    optionsMenu_.drawButton(context);

    drawViewModeBar(context);

    packDropdown_.drawMenu(context, packManager);
    scenarioDropdown_.drawMenu(context, scenarioView);
    objectivesDropdown_.drawMenu(context, scenarioView);
    optionsMenu_.drawMenu(context);
}
