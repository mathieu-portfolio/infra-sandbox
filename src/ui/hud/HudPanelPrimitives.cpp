#include "ui/hud/HudPanelPrimitives.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"

#include <algorithm>

namespace {
TopBarLayout topBarLayoutForWidth(int screenWidth)
{
    return computeTopBarLayout({0.0f, 0.0f, static_cast<float>(screenWidth), UiTheme::topBarHeight});
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
}

Rectangle hudScenarioDroplistBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).scenario;
}

Rectangle hudPackDroplistBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).pack;
}

Rectangle hudObjectivesDroplistBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).objectives;
}

Rectangle hudPhaseButtonBounds(int screenWidth)
{
    Rectangle phase = topBarLayoutForWidth(screenWidth).phase;
    phase.width = std::max(92.0f, phase.width - 82.0f);
    return phase;
}

Rectangle hudResetButtonBounds(int screenWidth)
{
    Rectangle phase = topBarLayoutForWidth(screenWidth).phase;
    return {phase.x + phase.width - 74.0f, phase.y, 74.0f, phase.height};
}

Rectangle hudFeedbackBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).feedback;
}

Rectangle hudHelpBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).help;
}

Rectangle hudOptionsButtonBounds(int screenWidth)
{
    return topBarLayoutForWidth(screenWidth).options;
}

Rectangle hudOptionsMenuBounds(int screenWidth, int screenHeight)
{
    constexpr float width = 300.0f;
    constexpr float height = 148.0f;
    return {
        static_cast<float>(screenWidth) * 0.5f - width * 0.5f,
        static_cast<float>(screenHeight) * 0.5f - height * 0.5f,
        width,
        height,
    };
}

bool hudPointInFieldOrMenu(Vector2 point, Rectangle field, Rectangle menu)
{
    return CheckCollisionPointRec(point, field) || CheckCollisionPointRec(point, menu);
}

void drawHudTopButton(Rectangle bounds, const char* label, bool active)
{
    DrawRectangleRounded(bounds, 0.16f, 6, active ? Color{37, 120, 255, 220} : Color{22, 27, 34, 235});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, active ? Color{89, 196, 255, 230} : Color{70, 86, 104, 120});
    const int textWidth = MeasureText(label, 14);
    DrawText(label, static_cast<int>(bounds.x + bounds.width * 0.5f - static_cast<float>(textWidth) * 0.5f), static_cast<int>(bounds.y + 8.0f), 14, active ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
}

void drawHudIconButton(Rectangle bounds, const char* iconId, bool active)
{
    DrawRectangleRounded(bounds, 0.16f, 6, active ? Color{37, 120, 255, 180} : Color{22, 27, 34, 200});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, active ? Color{89, 196, 255, 210} : Color{70, 86, 104, 95});
    IconRegistry::instance().drawIcon(iconId, {bounds.x + 7.0f, bounds.y + 7.0f, bounds.width - 14.0f, bounds.height - 14.0f}, active ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
}

void drawHudDroplistField(Rectangle bounds, const std::string& label, const std::string& value, bool open)
{
    const Color border = open ? Color{89, 196, 255, 230} : Color{70, 86, 104, 130};
    DrawRectangleRounded(bounds, 0.12f, 6, {22, 27, 34, 235});
    DrawRectangleRoundedLines(bounds, 0.12f, 6, border);
    DrawText(label.c_str(), static_cast<int>(bounds.x + 12.0f), static_cast<int>(bounds.y + 5.0f), 11, {89, 196, 255, 255});
    drawTextClipped(value, {bounds.x + 12.0f, bounds.y + 18.0f, bounds.width - 36.0f, 14.0f}, 13, {230, 237, 243, 255});
    drawChevron(bounds, open);
}

void drawHudMenuShell(Rectangle bounds)
{
    DrawRectangleRounded(bounds, 0.04f, 8, {14, 20, 28, 244});
    DrawRectangleRoundedLines(bounds, 0.04f, 8, {89, 196, 255, 150});
}

void drawHudDetailRow(const std::string& label, const std::string& value, float x, float y, float width)
{
    DrawText(label.c_str(), static_cast<int>(x), static_cast<int>(y), 12, {139, 148, 158, 255});
    drawTextClipped(value, {x + 86.0f, y - 1.0f, width - 98.0f, 16.0f}, 13, {230, 237, 243, 255});
}
