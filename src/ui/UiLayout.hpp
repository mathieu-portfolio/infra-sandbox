#pragma once

#include "raylib.h"

struct UiLayout {
    Rectangle topBar{};
    Rectangle leftSidebar{};
    Rectangle rightSidebar{};
    Rectangle bottomPanel{};
    Rectangle worldView{};
};

struct UiTheme {
    static constexpr float margin = 12.0f;
    static constexpr float gap = 10.0f;
    static constexpr float padding = 12.0f;
    static constexpr float topBarHeight = 54.0f;
    static constexpr float leftWidth = 250.0f;
    static constexpr float rightWidth = 336.0f;
    static constexpr float bottomHeight = 246.0f;
};

UiLayout computeUiLayout(int screenWidth, int screenHeight);
bool pointInUiPanel(Vector2 point, const UiLayout& layout);
