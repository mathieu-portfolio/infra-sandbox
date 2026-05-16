#include "ui/UiPrimitives.hpp"

std::string ellipsizeText(const std::string& text, int fontSize, float maxWidth)
{
    if (MeasureText(text.c_str(), fontSize) <= maxWidth) {
        return text;
    }

    std::string trimmed = text;
    while (!trimmed.empty()) {
        trimmed.pop_back();
        const std::string candidate = trimmed + "...";
        if (MeasureText(candidate.c_str(), fontSize) <= maxWidth) {
            return candidate;
        }
    }
    return "...";
}

void drawTextClipped(const std::string& text, Rectangle bounds, int fontSize, Color color)
{
    BeginScissorMode(
        static_cast<int>(bounds.x),
        static_cast<int>(bounds.y),
        static_cast<int>(bounds.width),
        static_cast<int>(bounds.height));
    const std::string visible = ellipsizeText(text, fontSize, bounds.width);
    DrawText(visible.c_str(), static_cast<int>(bounds.x), static_cast<int>(bounds.y), fontSize, color);
    EndScissorMode();
}

void drawPanelFrame(Rectangle bounds, const char* title)
{
    DrawRectangleRounded(bounds, 0.04f, 8, {22, 27, 34, 225});
    DrawRectangleRoundedLines(bounds, 0.04f, 8, {70, 86, 104, 95});
    drawTextClipped(title, {bounds.x + 12.0f, bounds.y + 10.0f, bounds.width - 24.0f, 20.0f}, 16, {230, 237, 243, 255});
}

void drawToggle(Rectangle bounds, bool enabled)
{
    const Color track = enabled ? Color{37, 120, 255, 215} : Color{50, 58, 70, 210};
    const Color knob = enabled ? Color{230, 237, 243, 255} : Color{139, 148, 158, 230};
    DrawRectangleRounded(bounds, 0.5f, 10, track);
    const float radius = bounds.height * 0.38f;
    const float x = enabled ? bounds.x + bounds.width - bounds.height * 0.5f : bounds.x + bounds.height * 0.5f;
    DrawCircleV({x, bounds.y + bounds.height * 0.5f}, radius, knob);
}
