#pragma once

#include "raylib.h"

#include <string>

std::string ellipsizeText(const std::string& text, int fontSize, float maxWidth);
float measureTextWrappedHeight(const std::string& text, float width, int fontSize, int maxLines, float lineSpacing = 3.0f);
void drawTextClipped(const std::string& text, Rectangle bounds, int fontSize, Color color);
void drawTextWrappedClipped(const std::string& text, Rectangle bounds, int fontSize, Color color, float lineSpacing = 3.0f);
void drawPanelFrame(Rectangle bounds, const char* title);
void drawToggle(Rectangle bounds, bool enabled);


struct UiIconLabelStyle {
    float iconSize = 16.0f;
    float gap = 8.0f;
    int fontSize = 13;
    Color iconColor{139, 148, 158, 255};
    Color textColor{230, 237, 243, 255};
};

Rectangle centeredSquare(Rectangle row, float size);
Rectangle centeredTextBounds(Rectangle row, float x, float width, int fontSize);
void drawIconLabelRow(Rectangle row, const std::string& iconId, const std::string& label, UiIconLabelStyle style = {});
void drawIconLabelRowValue(Rectangle row, const std::string& iconId, const std::string& label, const std::string& value, float valueWidth, UiIconLabelStyle style = {}, Color valueColor = Color{230, 237, 243, 255});
