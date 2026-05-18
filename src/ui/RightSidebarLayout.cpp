#include "ui/RightSidebarLayout.hpp"

#include "ui/UiNode.hpp"

namespace {
constexpr float kPad = 18.0f;
constexpr float kGap = 14.0f;
constexpr float kHeaderHeight = 64.0f;
constexpr float kTabsHeight = 46.0f;
constexpr float kOverviewHeight = 208.0f;
constexpr float kActionHeaderHeight = 190.0f;
constexpr float kActionMinHeight = 240.0f;
constexpr float kMessageHeight = 56.0f;
constexpr float kStatusMessageHeight = 24.0f;
constexpr float kSummaryMessageHeight = 20.0f;
constexpr float kLockedHeight = 72.0f;
constexpr float kButtonHeight = 46.0f;

Rectangle boundsOf(const ui::UiNode& root, const char* id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
}
}

RightSidebarLayout computeRightSidebarLayout(Rectangle sidebar)
{
    auto root = ui::verticalStack("root");
    ui::LayoutStyle rootStyle;
    rootStyle.paddingLeft = kPad;
    rootStyle.paddingTop = kPad;
    rootStyle.paddingRight = kPad;
    rootStyle.paddingBottom = kPad;
    rootStyle.gap = kGap;
    root->style(rootStyle);

    auto header = std::make_unique<ui::PanelNode>("header");
    header->style(ui::fixedHeight(kHeaderHeight));
    root->add(std::move(header));

    auto tabs = std::make_unique<ui::PanelNode>("tabs");
    tabs->style(ui::fixedHeight(kTabsHeight));
    root->add(std::move(tabs));

    auto overview = std::make_unique<ui::PanelNode>("overview");
    overview->style(ui::fixedHeight(kOverviewHeight));
    root->add(std::move(overview));

    auto actions = ui::verticalStack("actions");
    ui::LayoutStyle actionsStyle = ui::flex(1.0f, kActionMinHeight);
    actionsStyle.gap = 0.0f;
    actions->style(actionsStyle);
    auto actionHeader = std::make_unique<ui::PanelNode>("actionHeader");
    actionHeader->style(ui::fixedHeight(kActionHeaderHeight));
    actions->add(std::move(actionHeader));
    auto actionList = std::make_unique<ui::ScrollAreaNode>("actionList");
    actionList->style(ui::flex(1.0f, 0.0f));
    actions->add(std::move(actionList));
    root->add(std::move(actions));

    auto message = ui::verticalStack("message");
    ui::LayoutStyle messageStyle = ui::fixedHeight(kMessageHeight);
    messageStyle.gap = 4.0f;
    message->style(messageStyle);
    auto statusMessage = std::make_unique<ui::PanelNode>("statusMessage");
    statusMessage->style(ui::fixedHeight(kStatusMessageHeight));
    message->add(std::move(statusMessage));
    auto summaryMessage = std::make_unique<ui::PanelNode>("summaryMessage");
    summaryMessage->style(ui::fixedHeight(kSummaryMessageHeight));
    message->add(std::move(summaryMessage));
    root->add(std::move(message));

    auto locked = std::make_unique<ui::PanelNode>("locked");
    locked->style(ui::fixedHeight(kLockedHeight));
    root->add(std::move(locked));

    auto button = std::make_unique<ui::ButtonNode>("", 15, "button");
    button->style(ui::fixedHeight(kButtonHeight));
    root->add(std::move(button));

    root->measure({sidebar.width, sidebar.height});
    root->layout(sidebar);

    return {
        .root = sidebar,
        .header = boundsOf(*root, "header"),
        .tabs = boundsOf(*root, "tabs"),
        .overview = boundsOf(*root, "overview"),
        .actions = boundsOf(*root, "actions"),
        .actionHeader = boundsOf(*root, "actionHeader"),
        .actionList = boundsOf(*root, "actionList"),
        .message = boundsOf(*root, "message"),
        .statusMessage = boundsOf(*root, "statusMessage"),
        .summaryMessage = boundsOf(*root, "summaryMessage"),
        .locked = boundsOf(*root, "locked"),
        .button = boundsOf(*root, "button"),
    };
}
