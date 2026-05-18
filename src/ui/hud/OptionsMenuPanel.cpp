#include "ui/hud/OptionsMenuPanel.hpp"

#include "ui/hud/HudPanelPrimitives.hpp"
#include "ui/core/UiPrimitives.hpp"

namespace {
Rectangle optionsRowBounds(Rectangle menu, int index)
{
    return {menu.x + 10.0f, menu.y + 34.0f + static_cast<float>(index) * 29.0f, menu.width - 20.0f, 25.0f};
}

void drawOptionRow(Rectangle row, const char* label, bool enabled)
{
    DrawRectangleRounded(row, 0.12f, 6, {22, 27, 34, 175});
    DrawRectangleRoundedLines(row, 0.12f, 6, {70, 86, 104, 95});
    DrawText(label, static_cast<int>(row.x + 10.0f), static_cast<int>(row.y + 6.0f), 12, {230, 237, 243, 255});
    drawToggle({row.x + row.width - 44.0f, row.y + 4.0f, 34.0f, 17.0f}, enabled);
}
}

Rectangle OptionsMenuPanel::menuBounds(const UiContext& context) const
{
    return hudOptionsMenuBounds(context.screenWidth, context.screenHeight);
}

bool OptionsMenuPanel::update(UiContext& context, Vector2 mouse)
{
    if (context.state == nullptr) {
        return false;
    }

    const Rectangle button = hudOptionsButtonBounds(context.screenWidth);
    if (CheckCollisionPointRec(mouse, button)) {
        context.state->optionsMenuOpen = !context.state->optionsMenuOpen;
        context.state->scenarioDroplistOpen = false;
        context.state->objectivesDroplistOpen = false;
        return true;
    }

    if (!context.state->optionsMenuOpen) {
        return false;
    }

    const Rectangle menu = menuBounds(context);
    for (int i = 0; i < 4; ++i) {
        if (!CheckCollisionPointRec(mouse, optionsRowBounds(menu, i))) {
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
        return true;
    }

    if (!hudPointInFieldOrMenu(mouse, button, menu)) {
        context.state->optionsMenuOpen = false;
    }
    return false;
}

void OptionsMenuPanel::drawButton(const UiContext& context) const
{
    if (context.state == nullptr) {
        return;
    }
    drawHudIconButton(hudOptionsButtonBounds(context.screenWidth), "hud.options", context.state->optionsMenuOpen);
}

void OptionsMenuPanel::drawMenu(const UiContext& context) const
{
    if (context.state == nullptr || !context.state->optionsMenuOpen) {
        return;
    }

    const Rectangle menu = menuBounds(context);
    drawHudMenuShell(menu);
    DrawText("Options", static_cast<int>(menu.x + 12.0f), static_cast<int>(menu.y + 12.0f), 14, {89, 196, 255, 255});
    drawOptionRow(optionsRowBounds(menu, 0), "Maximized", IsWindowMaximized());
    drawOptionRow(optionsRowBounds(menu, 1), "Metrics panel", context.state->showMetrics);
    drawOptionRow(optionsRowBounds(menu, 2), "Debug UI", context.state->showDebug);
    drawOptionRow(optionsRowBounds(menu, 3), "Map grid", context.state->showGeoGrid);
}
