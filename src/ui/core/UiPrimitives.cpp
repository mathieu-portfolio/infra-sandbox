#include "ui/core/UiPrimitives.hpp"

#include <algorithm>

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

float measureTextWrappedHeight(const std::string& text, float width, int fontSize, int maxLines, float lineSpacing)
{
    if (text.empty()) {
        return static_cast<float>(fontSize);
    }

    const float avgCharWidth = static_cast<float>(fontSize) * 0.56f;
    const int charsPerLine = std::max(1, static_cast<int>(width / std::max(1.0f, avgCharWidth)));
    int lines = 1;
    int current = 0;

    for (char c : text) {
        if (c == '\n') {
            ++lines;
            current = 0;
            continue;
        }
        ++current;
        if (current >= charsPerLine) {
            ++lines;
            current = 0;
        }
    }

    const int visibleLines = std::clamp(lines, 1, std::max(1, maxLines));
    return static_cast<float>(visibleLines * fontSize) + std::max(0, visibleLines - 1) * lineSpacing;
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


void drawTextWrappedClipped(const std::string& text, Rectangle bounds, int fontSize, Color color, float lineSpacing)
{
    BeginScissorMode(
        static_cast<int>(bounds.x),
        static_cast<int>(bounds.y),
        static_cast<int>(bounds.width),
        static_cast<int>(bounds.height));

    const float lineHeight = static_cast<float>(fontSize) + lineSpacing;
    float x = bounds.x;
    float y = bounds.y;
    std::string line;
    std::string word;

    auto flushLine = [&]() {
        if (line.empty() || y + static_cast<float>(fontSize) > bounds.y + bounds.height) {
            return;
        }
        DrawText(line.c_str(), static_cast<int>(x), static_cast<int>(y), fontSize, color);
        y += lineHeight;
        line.clear();
    };

    auto pushWord = [&](const std::string& nextWord) {
        const std::string candidate = line.empty() ? nextWord : line + " " + nextWord;
        if (MeasureText(candidate.c_str(), fontSize) <= bounds.width) {
            line = candidate;
            return;
        }
        flushLine();
        line = nextWord;
    };

    for (const char ch : text) {
        if (ch == '\n') {
            if (!word.empty()) {
                pushWord(word);
                word.clear();
            }
            flushLine();
            continue;
        }
        if (ch == ' ' || ch == '\t') {
            if (!word.empty()) {
                pushWord(word);
                word.clear();
            }
            continue;
        }
        word.push_back(ch);
    }
    if (!word.empty()) {
        pushWord(word);
    }
    flushLine();

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
