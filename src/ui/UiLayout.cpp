#include "ui/UiLayout.hpp"

#include <algorithm>

UiLayout computeUiLayout(int screenWidth, int screenHeight)
{
    const float width = static_cast<float>(screenWidth);
    const float height = static_cast<float>(screenHeight);
    const float leftWidth = std::min(UiTheme::leftWidth, width * 0.22f);
    const float rightWidth = std::min(UiTheme::rightWidth, width * 0.40f);
    const float bottomHeight = std::min(UiTheme::bottomHeight, height * 0.45f);

    UiLayout layout;
    layout.topBar = {0.0f, 0.0f, width, UiTheme::topBarHeight};
    layout.leftSidebar = {UiTheme::margin, UiTheme::topBarHeight + UiTheme::margin, leftWidth, height - UiTheme::topBarHeight - UiTheme::margin * 2.0f};
    layout.rightSidebar = {width - rightWidth - UiTheme::margin, UiTheme::topBarHeight + UiTheme::margin, rightWidth, height - UiTheme::topBarHeight - UiTheme::margin * 2.0f};
    layout.bottomPanel = {
        layout.leftSidebar.x + layout.leftSidebar.width + UiTheme::gap,
        height - bottomHeight - UiTheme::margin,
        layout.rightSidebar.x - (layout.leftSidebar.x + layout.leftSidebar.width) - UiTheme::gap * 2.0f,
        bottomHeight,
    };
    layout.worldView = {
        layout.leftSidebar.x + layout.leftSidebar.width + UiTheme::gap,
        UiTheme::topBarHeight + UiTheme::margin,
        layout.rightSidebar.x - (layout.leftSidebar.x + layout.leftSidebar.width) - UiTheme::gap * 2.0f,
        layout.bottomPanel.y - (UiTheme::topBarHeight + UiTheme::margin) - UiTheme::gap,
    };
    return layout;
}

bool pointInUiPanel(Vector2 point, const UiLayout& layout)
{
    return CheckCollisionPointRec(point, layout.topBar)
        || CheckCollisionPointRec(point, layout.leftSidebar)
        || CheckCollisionPointRec(point, layout.rightSidebar)
        || CheckCollisionPointRec(point, layout.bottomPanel);
}
