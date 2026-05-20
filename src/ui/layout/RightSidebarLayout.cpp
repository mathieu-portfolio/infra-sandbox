#include "ui/layout/RightSidebarLayout.hpp"

#include <algorithm>

namespace {
constexpr float kPad = 18.0f;
constexpr float kGap = 14.0f;
constexpr float kHeaderHeight = 64.0f;
constexpr float kTabsHeight = 46.0f;
constexpr float kOverviewHeight = 208.0f;
constexpr float kActionHeaderHeight = 186.0f;
constexpr float kMessageHeight = 56.0f;
constexpr float kStatusMessageHeight = 24.0f;
constexpr float kSummaryMessageHeight = 20.0f;
constexpr float kLockedHeight = 72.0f;
constexpr float kButtonHeight = 46.0f;

Rectangle takeTop(float x, float width, float& y, float height)
{
    const Rectangle rect{x, y, width, std::max(0.0f, height)};
    y += rect.height + kGap;
    return rect;
}
}

RightSidebarLayout computeRightSidebarLayout(Rectangle sidebar)
{
    const float x = sidebar.x + kPad;
    const float width = std::max(0.0f, sidebar.width - kPad * 2.0f);
    const float top = sidebar.y + kPad;
    const float bottom = sidebar.y + sidebar.height - kPad;

    float y = top;
    RightSidebarLayout layout{};
    layout.root = sidebar;
    layout.header = takeTop(x, width, y, kHeaderHeight);
    layout.tabs = takeTop(x, width, y, kTabsHeight);
    layout.overview = takeTop(x, width, y, kOverviewHeight);

    const float footerHeight = kMessageHeight + kGap + kLockedHeight + kGap + kButtonHeight;
    const float footerTop = std::max(y, bottom - footerHeight);
    const float actionAreaHeight = std::max(0.0f, footerTop - y - kGap);
    const float actionHeaderHeight = std::min(kActionHeaderHeight, actionAreaHeight);
    const float actionListHeight = std::max(0.0f, actionAreaHeight - actionHeaderHeight);

    layout.actions = {x, y, width, std::max(0.0f, actionHeaderHeight + actionListHeight)};
    layout.actionHeader = {x, y, width, actionHeaderHeight};
    layout.actionList = {x, y + actionHeaderHeight, width, actionListHeight};

    y = footerTop;
    layout.message = takeTop(x, width, y, kMessageHeight);
    layout.statusMessage = {layout.message.x, layout.message.y, layout.message.width, kStatusMessageHeight};
    layout.summaryMessage = {
        layout.message.x,
        layout.message.y + kStatusMessageHeight + 4.0f,
        layout.message.width,
        kSummaryMessageHeight,
    };
    layout.locked = takeTop(x, width, y, kLockedHeight);
    layout.button = {x, y, width, kButtonHeight};

    return layout;
}
