#include "ui/actions/ActionText.hpp"

namespace actions_ui {

void drawWrappedTextClipped(const std::string& text, Rectangle bounds, int fontSize, Color color, float lineSpacing)
{
    if (text.empty() || bounds.width <= 0.0f || bounds.height <= 0.0f) {
        return;
    }

    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height));

    float y = bounds.y;
    std::string line;
    std::string word;

    auto flushWord = [&]() {
        if (word.empty()) {
            return;
        }
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && MeasureText(candidate.c_str(), fontSize) > bounds.width) {
            if (y + static_cast<float>(fontSize) > bounds.y + bounds.height) {
                word.clear();
                return;
            }
            DrawText(line.c_str(), static_cast<int>(bounds.x), static_cast<int>(y), fontSize, color);
            y += static_cast<float>(fontSize) + lineSpacing;
            line = word;
        } else {
            line = candidate;
        }
        word.clear();
    };

    for (char c : text) {
        if (c == ' ' || c == '\n' || c == '\t') {
            flushWord();
            if (c == '\n') {
                if (!line.empty() && y + static_cast<float>(fontSize) <= bounds.y + bounds.height) {
                    DrawText(line.c_str(), static_cast<int>(bounds.x), static_cast<int>(y), fontSize, color);
                }
                y += static_cast<float>(fontSize) + lineSpacing;
                line.clear();
            }
        } else {
            word.push_back(c);
        }
    }
    flushWord();

    if (!line.empty() && y + static_cast<float>(fontSize) <= bounds.y + bounds.height) {
        DrawText(line.c_str(), static_cast<int>(bounds.x), static_cast<int>(y), fontSize, color);
    }

    EndScissorMode();
}

}
