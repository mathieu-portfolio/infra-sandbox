#include "ui/core/ScrollHandling.hpp"

#include <algorithm>
#include <cmath>

namespace ui {


namespace {
std::vector<Rectangle>& scissorStack()
{
    static std::vector<Rectangle> stack;
    return stack;
}

Rectangle normalized(Rectangle bounds)
{
    const float x = std::floor(bounds.x);
    const float y = std::floor(bounds.y);
    const float right = std::ceil(bounds.x + bounds.width);
    const float bottom = std::ceil(bounds.y + bounds.height);
    return {x, y, std::max(0.0f, right - x), std::max(0.0f, bottom - y)};
}

Rectangle intersect(Rectangle a, Rectangle b)
{
    const float left = std::max(a.x, b.x);
    const float top = std::max(a.y, b.y);
    const float right = std::min(a.x + a.width, b.x + b.width);
    const float bottom = std::min(a.y + a.height, b.y + b.height);
    return {left, top, std::max(0.0f, right - left), std::max(0.0f, bottom - top)};
}

void applyScissor(Rectangle bounds)
{
    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height));
}
} // namespace

ScissorGuard::ScissorGuard(Rectangle bounds)
{
    Rectangle clipped = normalized(bounds);
    auto& stack = scissorStack();
    if (!stack.empty()) {
        clipped = intersect(stack.back(), clipped);
    }
    stack.push_back(clipped);
    applyScissor(clipped);
    active_ = true;
}

ScissorGuard::~ScissorGuard()
{
    if (!active_) {
        return;
    }
    auto& stack = scissorStack();
    if (!stack.empty()) {
        stack.pop_back();
    }
    if (stack.empty()) {
        EndScissorMode();
    } else {
        applyScissor(stack.back());
    }
}

float maxScrollOffset(float contentHeight, float viewportHeight)
{
    return std::max(0.0f, contentHeight - viewportHeight);
}

float clampScrollOffset(float offset, float contentHeight, float viewportHeight)
{
    return std::clamp(offset, 0.0f, maxScrollOffset(contentHeight, viewportHeight));
}

Rectangle scrollViewport(Rectangle bounds, float topInset, float bottomInset, float horizontalInset)
{
    const float insetX = std::max(0.0f, horizontalInset);
    const float insetTop = std::max(0.0f, topInset);
    const float insetBottom = std::max(0.0f, bottomInset);
    return {
        bounds.x + insetX,
        bounds.y + insetTop,
        std::max(0.0f, bounds.width - insetX * 2.0f),
        std::max(0.0f, bounds.height - insetTop - insetBottom),
    };
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
