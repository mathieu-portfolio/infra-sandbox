#pragma once

#include "raylib.h"

#include <string>

namespace actions_ui {
void drawWrappedTextClipped(const std::string& text, Rectangle bounds, int fontSize, Color color, float lineSpacing = 4.0f);
}
