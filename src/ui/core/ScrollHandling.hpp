#pragma once

#include "raylib.h"

#include <vector>

namespace ui {

struct ScrollResult {
    bool hovered = false;
    bool changed = false;
};

class ScissorGuard {
public:
    explicit ScissorGuard(Rectangle bounds);
    ~ScissorGuard();

    ScissorGuard(const ScissorGuard&) = delete;
    ScissorGuard& operator=(const ScissorGuard&) = delete;

private:
    bool active_ = false;
};

float maxScrollOffset(float contentHeight, float viewportHeight);
float clampScrollOffset(float offset, float contentHeight, float viewportHeight);
Rectangle scrollViewport(Rectangle bounds, float topInset, float bottomInset = 0.0f, float horizontalInset = 0.0f);
ScrollResult updateScrollOffset(Rectangle viewport, float contentHeight, float wheel, Vector2 mouse, float& offset, float pixelsPerWheel = 42.0f);

} // namespace ui
