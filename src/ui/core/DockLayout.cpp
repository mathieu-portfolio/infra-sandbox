#include "ui/core/DockLayout.hpp"

#include "ui/core/UiLayout.hpp"


#include <algorithm>

namespace {
float maxLeftWidth(float screenWidth, float rightWidth, const DockLayoutConfig& config)
{
    const float ratioMax = screenWidth * config.maxLeftWidthRatio;
    const float worldMax = screenWidth - UiTheme::margin * 2.0f - UiTheme::gap * 2.0f - rightWidth - config.minWorldWidth;
    return std::max(config.minLeftWidth, std::min(ratioMax, worldMax));
}

float maxRightWidth(float screenWidth, float leftWidth, const DockLayoutConfig& config)
{
    const float ratioMax = screenWidth * config.maxRightWidthRatio;
    const float worldMax = screenWidth - UiTheme::margin * 2.0f - UiTheme::gap * 2.0f - leftWidth - config.minWorldWidth;
    return std::max(config.minRightWidth, std::min(ratioMax, worldMax));
}

float maxBottomHeight(float screenHeight, const DockLayoutConfig& config)
{
    const float bodyHeight = screenHeight - UiTheme::topBarHeight - UiTheme::margin * 2.0f;
    const float ratioMax = screenHeight * config.maxBottomHeightRatio;
    const float worldMax = bodyHeight - UiTheme::gap - config.minWorldHeight;
    return std::max(config.minBottomHeight, std::min(ratioMax, worldMax));
}

float centerY(UiRect rect)
{
    return rect.y + rect.height * 0.5f;
}
}

void clampDockLayoutState(DockLayoutState& state, int screenWidth, int screenHeight, const DockLayoutConfig& config)
{
    const float width = static_cast<float>(screenWidth);
    const float height = static_cast<float>(screenHeight);

    state.leftWidth = std::clamp(state.leftWidth, config.minLeftWidth, maxLeftWidth(width, state.rightWidth, config));
    state.rightWidth = std::clamp(state.rightWidth, config.minRightWidth, maxRightWidth(width, state.leftWidth, config));
    state.leftWidth = std::clamp(state.leftWidth, config.minLeftWidth, maxLeftWidth(width, state.rightWidth, config));
    state.bottomHeight = std::clamp(state.bottomHeight, config.minBottomHeight, maxBottomHeight(height, config));
}

DockLayoutFrame computeDockLayout(const DockLayoutState& sourceState, int screenWidth, int screenHeight, const DockLayoutConfig& config)
{
    DockLayoutState state = sourceState;
    clampDockLayoutState(state, screenWidth, screenHeight, config);

    const float width = static_cast<float>(screenWidth);
    const float height = static_cast<float>(screenHeight);
    const float bodyY = UiTheme::topBarHeight + UiTheme::margin;
    const float bodyHeight = std::max(0.0f, height - bodyY - UiTheme::margin);
    const float bodyX = UiTheme::margin;
    const float bodyWidth = std::max(0.0f, width - UiTheme::margin * 2.0f);

    const UiRect topBar{0.0f, 0.0f, width, UiTheme::topBarHeight};
    const UiRect left{bodyX, bodyY, state.leftWidth, bodyHeight};
    const UiRect right{bodyX + bodyWidth - state.rightWidth, bodyY, state.rightWidth, bodyHeight};
    const float centerX = left.x + left.width + UiTheme::gap;
    const float centerRight = right.x - UiTheme::gap;
    const float centerWidth = std::max(0.0f, centerRight - centerX);
    const UiRect bottom{centerX, bodyY + bodyHeight - state.bottomHeight, centerWidth, state.bottomHeight};
    const UiRect world{centerX, bodyY, centerWidth, std::max(0.0f, bottom.y - UiTheme::gap - bodyY)};

    const float handle = config.handleThickness;
    return {
        .topBar = topBar,
        .leftSidebar = left,
        .rightSidebar = right,
        .bottomPanel = bottom,
        .worldView = world,
        .leftResizeHandle = {left.x + left.width - handle * 0.5f, left.y, handle, left.height},
        .rightResizeHandle = {right.x - handle * 0.5f, right.y, handle, right.height},
        .bottomResizeHandle = {bottom.x, bottom.y - handle * 0.5f, bottom.width, handle},
    };
}

DockResizeHandle dockResizeHandleAt(UiPoint point, const DockLayoutFrame& frame)
{
    if (contains(frame.leftResizeHandle, point)) {
        return DockResizeHandle::LeftRightEdge;
    }
    if (contains(frame.rightResizeHandle, point)) {
        return DockResizeHandle::RightLeftEdge;
    }
    if (contains(frame.bottomResizeHandle, point)) {
        return DockResizeHandle::BottomTopEdge;
    }
    return DockResizeHandle::None;
}

DockLayoutFrame DockLayoutController::compute(const DockLayoutState& state, int screenWidth, int screenHeight) const
{
    return computeDockLayout(state, screenWidth, screenHeight, config_);
}

void DockLayoutController::update(DockLayoutState& state, int screenWidth, int screenHeight, UiPoint mouse, bool mousePressed, bool mouseDown) const
{
    clampDockLayoutState(state, screenWidth, screenHeight, config_);

    const DockLayoutFrame frame = compute(state, screenWidth, screenHeight);
    state.hoveredHandle = dockResizeHandleAt(mouse, frame);

    if (mousePressed && state.hoveredHandle != DockResizeHandle::None) {
        state.activeHandle = state.hoveredHandle;
        state.dragStartMouse = mouse;
        switch (state.activeHandle) {
        case DockResizeHandle::LeftRightEdge:
            state.dragStartSize = state.leftWidth;
            break;
        case DockResizeHandle::RightLeftEdge:
            state.dragStartSize = state.rightWidth;
            break;
        case DockResizeHandle::BottomTopEdge:
            state.dragStartSize = state.bottomHeight;
            break;
        case DockResizeHandle::None:
            state.dragStartSize = 0.0f;
            break;
        }
    }

    if (state.activeHandle != DockResizeHandle::None) {
        if (!mouseDown) {
            state.activeHandle = DockResizeHandle::None;
            state.dragStartSize = 0.0f;
        } else {
            switch (state.activeHandle) {
            case DockResizeHandle::LeftRightEdge:
                state.leftWidth = state.dragStartSize + (mouse.x - state.dragStartMouse.x);
                break;
            case DockResizeHandle::RightLeftEdge:
                state.rightWidth = state.dragStartSize - (mouse.x - state.dragStartMouse.x);
                break;
            case DockResizeHandle::BottomTopEdge:
                state.bottomHeight = state.dragStartSize - (mouse.y - state.dragStartMouse.y);
                break;
            case DockResizeHandle::None:
                break;
            }
            clampDockLayoutState(state, screenWidth, screenHeight, config_);
        }
    }
}

bool DockLayoutController::isMouseOwnedByDock(const DockLayoutState& state) const
{
    return state.activeHandle != DockResizeHandle::None || state.hoveredHandle != DockResizeHandle::None;
}
