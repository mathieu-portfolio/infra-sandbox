#include "ui/core/ScrollHandling.hpp"

#include <algorithm>

namespace ui {

float maxScrollOffset(float contentHeight, float viewportHeight)
{
    return std::max(0.0f, contentHeight - viewportHeight);
}

float clampScrollOffset(float offset, float contentHeight, float viewportHeight)
{
    return std::clamp(offset, 0.0f, maxScrollOffset(contentHeight, viewportHeight));
}

ScrollResult updateScrollOffset(Rectangle viewport, float contentHeight, float wheel, Vector2 mouse, float& offset, float pixelsPerWheel)
{
    const bool hovered = CheckCollisionPointRec(mouse, viewport);
    if (!hovered) {
        offset = clampScrollOffset(offset, contentHeight, viewport.height);
        return {.hovered = false, .changed = false};
    }

    const float previous = offset;
    if (wheel != 0.0f) {
        offset = clampScrollOffset(offset - wheel * pixelsPerWheel, contentHeight, viewport.height);
    } else {
        offset = clampScrollOffset(offset, contentHeight, viewport.height);
    }
    return {.hovered = true, .changed = offset != previous};
}

} // namespace ui
