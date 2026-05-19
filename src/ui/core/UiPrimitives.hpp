#pragma once

#include "raylib.h"

#include <string>

std::string ellipsizeText(const std::string& text, int fontSize, float maxWidth);
float measureTextWrappedHeight(const std::string& text, float width, int fontSize, int maxLines, float lineSpacing = 3.0f);
void drawTextClipped(const std::string& text, Rectangle bounds, int fontSize, Color color);
void drawTextWrappedClipped(const std::string& text, Rectangle bounds, int fontSize, Color color, float lineSpacing = 3.0f);
void drawPanelFrame(Rectangle bounds, const char* title);
void drawToggle(Rectangle bounds, bool enabled);
