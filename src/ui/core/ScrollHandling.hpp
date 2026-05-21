#pragma once

#include "raylib.h"

namespace ui {

struct ScrollResult {
    bool hovered = false;
    bool changed = false;
};

float maxScrollOffset(float contentHeight, float viewportHeight);
float clampScrollOffset(float offset, float contentHeight, float viewportHeight);
ScrollResult updateScrollOffset(Rectangle viewport, float contentHeight, float wheel, Vector2 mouse, float& offset, float pixelsPerWheel = 42.0f);

} // namespace ui
