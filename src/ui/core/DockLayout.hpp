#pragma once

#include "ui/core/UiGeometry.hpp"

struct DockLayoutConfig {
    float minLeftWidth = 220.0f;
    float minRightWidth = 360.0f;
    float minBottomHeight = 160.0f;
    float maxLeftWidthRatio = 0.34f;
    float maxRightWidthRatio = 0.48f;
    float maxBottomHeightRatio = 0.55f;
    float minWorldWidth = 360.0f;
    float minWorldHeight = 180.0f;
    float handleThickness = 8.0f;
};

enum class DockResizeHandle {
    None,
    LeftRightEdge,
    RightLeftEdge,
    BottomTopEdge,
};

struct DockLayoutState {
    float leftWidth = 250.0f;
    float rightWidth = 520.0f;
    float bottomHeight = 344.0f;
    DockResizeHandle hoveredHandle = DockResizeHandle::None;
    DockResizeHandle activeHandle = DockResizeHandle::None;
    UiPoint dragStartMouse{};
    float dragStartSize = 0.0f;
};

struct DockLayoutFrame {
    UiRect topBar{};
    UiRect leftSidebar{};
    UiRect rightSidebar{};
    UiRect bottomPanel{};
    UiRect worldView{};
    UiRect leftResizeHandle{};
    UiRect rightResizeHandle{};
    UiRect bottomResizeHandle{};
};

class DockLayoutController {
public:
    void update(DockLayoutState& state, int screenWidth, int screenHeight, UiPoint mouse, bool mousePressed, bool mouseDown) const;
    [[nodiscard]] DockLayoutFrame compute(const DockLayoutState& state, int screenWidth, int screenHeight) const;
    [[nodiscard]] bool isMouseOwnedByDock(const DockLayoutState& state) const;
    [[nodiscard]] const DockLayoutConfig& config() const { return config_; }

private:
    DockLayoutConfig config_{};
};

[[nodiscard]] DockLayoutFrame computeDockLayout(const DockLayoutState& state, int screenWidth, int screenHeight, const DockLayoutConfig& config = {});
[[nodiscard]] DockResizeHandle dockResizeHandleAt(UiPoint point, const DockLayoutFrame& frame);
void clampDockLayoutState(DockLayoutState& state, int screenWidth, int screenHeight, const DockLayoutConfig& config = {});
