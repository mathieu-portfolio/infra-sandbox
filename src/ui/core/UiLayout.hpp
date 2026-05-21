#pragma once

#include "raylib.h"

struct UiLayout {
    Rectangle topBar{};
    Rectangle leftSidebar{};
    Rectangle rightSidebar{};
    Rectangle bottomPanel{};
    Rectangle worldView{};
};

struct TopBarLayout {
    Rectangle root{};
    Rectangle brand{};
    Rectangle pack{};
    Rectangle scenario{};
    Rectangle time{};
    Rectangle phase{};
    Rectangle phaseLabel{};
    Rectangle objectives{};
    Rectangle feedback{};
    Rectangle help{};
    Rectangle options{};
};

struct LeftSidebarLayout {
    Rectangle root{};
    Rectangle overview{};
    Rectangle specializations{};
    Rectangle alerts{};
    Rectangle sandbox{};
    Rectangle legend{};
};

struct BottomPanelLayout {
    Rectangle root{};
    Rectangle title{};
    Rectangle charts{};
    Rectangle timelineHeader{};
    Rectangle categoryFilter{};
    Rectangle orderFilter{};
    Rectangle rows{};
};

struct UiTheme {
    static constexpr float margin = 12.0f;
    static constexpr float gap = 10.0f;
    static constexpr float padding = 12.0f;
    static constexpr float topBarHeight = 88.0f;
    static constexpr float leftWidth = 250.0f;
    static constexpr float rightWidth = 520.0f;
    static constexpr float bottomHeight = 344.0f;
};

UiLayout computeUiLayout(int screenWidth, int screenHeight);
TopBarLayout computeTopBarLayout(Rectangle topBar);
LeftSidebarLayout computeLeftSidebarLayout(Rectangle leftSidebar, bool sandboxMode);
BottomPanelLayout computeBottomPanelLayout(Rectangle bottomPanel);
Rectangle computeViewModeBarBounds(Rectangle worldView);
Rectangle computeViewModeButtonBounds(Rectangle viewModeBar, int index, int count);
bool pointInUiPanel(Vector2 point, const UiLayout& layout);
