#pragma once

#include "raylib.h"

#include <string>

std::string ellipsizeText(const std::string& text, int fontSize, float maxWidth);
void drawTextClipped(const std::string& text, Rectangle bounds, int fontSize, Color color);
void drawPanelFrame(Rectangle bounds, const char* title);
void drawToggle(Rectangle bounds, bool enabled);
