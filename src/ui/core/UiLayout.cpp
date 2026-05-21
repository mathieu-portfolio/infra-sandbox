#include "ui/core/UiLayout.hpp"

#include "ui/core/UiCore.hpp"

#include <algorithm>

namespace {
Rectangle boundsOf(const ui::UiNode& root, const char* id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
}

Rectangle centeredTopBarField(Rectangle bounds)
{
    constexpr float height = 34.0f;
    return {bounds.x, bounds.y + (bounds.height - height) * 0.5f, bounds.width, height};
}
}

UiLayout computeUiLayout(int screenWidth, int screenHeight)
{
    const float width = static_cast<float>(screenWidth);
    const float height = static_cast<float>(screenHeight);
    const float leftWidth = std::min(UiTheme::leftWidth, width * 0.22f);
    const float rightWidth = std::min(UiTheme::rightWidth, width * 0.40f);
    const float bottomHeight = std::min(UiTheme::bottomHeight, height * 0.45f);

    auto root = ui::verticalStack("root");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = UiTheme::margin;
    root->style(rootStyle);

    auto topBar = std::make_unique<ui::PanelNode>("topBar");
    topBar->style(ui::fixedHeight(UiTheme::topBarHeight));
    root->add(std::move(topBar));

    auto body = ui::horizontalStack("body");
    ui::LayoutStyle bodyStyle;
    bodyStyle.paddingLeft = UiTheme::margin;
    bodyStyle.paddingRight = UiTheme::margin;
    bodyStyle.paddingBottom = UiTheme::margin;
    bodyStyle.gap = UiTheme::gap;
    bodyStyle.flexGrow = 1.0f;
    bodyStyle.heightMode = ui::SizeMode::Flex;
    body->style(bodyStyle);

    auto left = std::make_unique<ui::PanelNode>("leftSidebar");
    ui::LayoutStyle leftStyle;
    leftStyle.fixedWidth = leftWidth;
    leftStyle.flexGrow = 1.0f;
    leftStyle.widthMode = ui::SizeMode::Fixed;
    leftStyle.heightMode = ui::SizeMode::Flex;
    left->style(leftStyle);
    body->add(std::move(left));

    auto center = ui::verticalStack("center");
    ui::LayoutStyle centerStyle;
    centerStyle.gap = UiTheme::gap;
    centerStyle.flexGrow = 1.0f;
    centerStyle.widthMode = ui::SizeMode::Flex;
    centerStyle.heightMode = ui::SizeMode::Flex;
    center->style(centerStyle);
    auto world = std::make_unique<ui::PanelNode>("worldView");
    ui::LayoutStyle worldStyle;
    worldStyle.minHeight = 120.0f;
    worldStyle.flexGrow = 1.0f;
    worldStyle.heightMode = ui::SizeMode::Flex;
    world->style(worldStyle);
    center->add(std::move(world));
    auto bottom = std::make_unique<ui::PanelNode>("bottomPanel");
    bottom->style(ui::fixedHeight(bottomHeight));
    center->add(std::move(bottom));
    body->add(std::move(center));

    auto right = std::make_unique<ui::PanelNode>("rightSidebar");
    ui::LayoutStyle rightStyle;
    rightStyle.fixedWidth = rightWidth;
    rightStyle.flexGrow = 1.0f;
    rightStyle.widthMode = ui::SizeMode::Fixed;
    rightStyle.heightMode = ui::SizeMode::Flex;
    right->style(rightStyle);
    body->add(std::move(right));

    root->add(std::move(body));
    root->measure({width, height});
    root->layout({0.0f, 0.0f, width, height});

    return {
        .topBar = boundsOf(*root, "topBar"),
        .leftSidebar = boundsOf(*root, "leftSidebar"),
        .rightSidebar = boundsOf(*root, "rightSidebar"),
        .bottomPanel = boundsOf(*root, "bottomPanel"),
        .worldView = boundsOf(*root, "worldView"),
    };
}

TopBarLayout computeTopBarLayout(Rectangle topBar)
{
    auto root = ui::horizontalStack("root");
    ui::LayoutStyle rootStyle;
    rootStyle.paddingLeft = 14.0f;
    rootStyle.paddingTop = 10.0f;
    rootStyle.paddingRight = 14.0f;
    rootStyle.paddingBottom = 10.0f;
    rootStyle.gap = 12.0f;
    root->style(rootStyle);

    auto brand = std::make_unique<ui::PanelNode>("brand");
    ui::LayoutStyle brandStyle;
    brandStyle.fixedWidth = 180.0f;
    brandStyle.flexGrow = 1.0f;
    brandStyle.widthMode = ui::SizeMode::Fixed;
    brandStyle.heightMode = ui::SizeMode::Flex;
    brand->style(brandStyle);
    root->add(std::move(brand));
    auto scenarioStack = std::make_unique<ui::PanelNode>("scenarioStack");
    ui::LayoutStyle scenarioStyle = brandStyle;
    scenarioStyle.fixedWidth = 270.0f;
    scenarioStack->style(scenarioStyle);
    root->add(std::move(scenarioStack));
    auto time = std::make_unique<ui::PanelNode>("time");
    ui::LayoutStyle timeStyle = brandStyle;
    timeStyle.fixedWidth = 154.0f;
    time->style(timeStyle);
    root->add(std::move(time));
    auto phase = std::make_unique<ui::PanelNode>("phase");
    ui::LayoutStyle phaseStyle = brandStyle;
    phaseStyle.fixedWidth = 204.0f;
    phase->style(phaseStyle);
    root->add(std::move(phase));
    auto phaseLabel = std::make_unique<ui::PanelNode>("phaseLabel");
    ui::LayoutStyle phaseLabelStyle = brandStyle;
    phaseLabelStyle.fixedWidth = 92.0f;
    phaseLabel->style(phaseLabelStyle);
    root->add(std::move(phaseLabel));
    auto objectives = std::make_unique<ui::PanelNode>("objectives");
    ui::LayoutStyle objectivesStyle;
    objectivesStyle.minWidth = 190.0f;
    objectivesStyle.flexGrow = 1.0f;
    objectivesStyle.widthMode = ui::SizeMode::Flex;
    objectivesStyle.heightMode = ui::SizeMode::Flex;
    objectives->style(objectivesStyle);
    root->add(std::move(objectives));
    auto feedback = std::make_unique<ui::PanelNode>("feedback");
    ui::LayoutStyle feedbackStyle = brandStyle;
    feedbackStyle.fixedWidth = 98.0f;
    feedback->style(feedbackStyle);
    root->add(std::move(feedback));
    auto help = std::make_unique<ui::PanelNode>("help");
    ui::LayoutStyle helpStyle = brandStyle;
    helpStyle.fixedWidth = 62.0f;
    help->style(helpStyle);
    root->add(std::move(help));
    auto options = std::make_unique<ui::PanelNode>("options");
    ui::LayoutStyle optionsStyle = brandStyle;
    optionsStyle.fixedWidth = 34.0f;
    options->style(optionsStyle);
    root->add(std::move(options));

    root->measure({topBar.width, topBar.height});
    root->layout(topBar);
    const Rectangle scenarioStackBounds = boundsOf(*root, "scenarioStack");
    return {
        .root = topBar,
        .brand = centeredTopBarField(boundsOf(*root, "brand")),
        .pack = {scenarioStackBounds.x, scenarioStackBounds.y, scenarioStackBounds.width, 32.0f},
        .scenario = {scenarioStackBounds.x, scenarioStackBounds.y + 36.0f, scenarioStackBounds.width, 32.0f},
        .time = centeredTopBarField(boundsOf(*root, "time")),
        .phase = centeredTopBarField(boundsOf(*root, "phase")),
        .phaseLabel = centeredTopBarField(boundsOf(*root, "phaseLabel")),
        .objectives = centeredTopBarField(boundsOf(*root, "objectives")),
        .feedback = centeredTopBarField(boundsOf(*root, "feedback")),
        .help = centeredTopBarField(boundsOf(*root, "help")),
        .options = centeredTopBarField(boundsOf(*root, "options")),
    };
}

LeftSidebarLayout computeLeftSidebarLayout(Rectangle leftSidebar, bool sandboxMode)
{
    auto root = ui::verticalStack("root");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = UiTheme::gap;
    root->style(rootStyle);
    auto overview = std::make_unique<ui::PanelNode>("overview");
    overview->style(ui::fixedHeight(250.0f));
    root->add(std::move(overview));
    auto alerts = std::make_unique<ui::PanelNode>("alerts");
    alerts->style(ui::fixedHeight(142.0f));
    root->add(std::move(alerts));
    if (sandboxMode) {
        auto sandbox = std::make_unique<ui::PanelNode>("sandbox");
        sandbox->style(ui::fixedHeight(288.0f));
        root->add(std::move(sandbox));
    }
    auto legend = std::make_unique<ui::PanelNode>("legend");
    legend->style(ui::flex(1.0f, 130.0f));
    root->add(std::move(legend));
    root->measure({leftSidebar.width, leftSidebar.height});
    root->layout(leftSidebar);
    return {
        .root = leftSidebar,
        .overview = boundsOf(*root, "overview"),
        .alerts = boundsOf(*root, "alerts"),
        .sandbox = boundsOf(*root, "sandbox"),
        .legend = boundsOf(*root, "legend"),
    };
}

BottomPanelLayout computeBottomPanelLayout(Rectangle bottomPanel)
{
    const float chartHeight = std::clamp(bottomPanel.height * 0.37f, 92.0f, 128.0f);
    auto root = ui::verticalStack("root");
    ui::LayoutStyle rootStyle;
    rootStyle.paddingLeft = 12.0f;
    rootStyle.paddingTop = 10.0f;
    rootStyle.paddingRight = 12.0f;
    rootStyle.paddingBottom = 12.0f;
    rootStyle.gap = 12.0f;
    root->style(rootStyle);
    auto title = std::make_unique<ui::PanelNode>("title");
    title->style(ui::fixedHeight(16.0f));
    root->add(std::move(title));
    auto charts = std::make_unique<ui::PanelNode>("charts");
    charts->style(ui::fixedHeight(chartHeight));
    root->add(std::move(charts));
    auto timelineHeader = std::make_unique<ui::PanelNode>("timelineHeader");
    timelineHeader->style(ui::fixedHeight(28.0f));
    root->add(std::move(timelineHeader));
    auto rows = std::make_unique<ui::PanelNode>("rows");
    rows->style(ui::flex(1.0f, 60.0f));
    root->add(std::move(rows));
    root->measure({bottomPanel.width, bottomPanel.height});
    root->layout(bottomPanel);

    const Rectangle header = boundsOf(*root, "timelineHeader");
    return {
        .root = bottomPanel,
        .title = boundsOf(*root, "title"),
        .charts = boundsOf(*root, "charts"),
        .timelineHeader = header,
        .categoryFilter = {header.x + header.width - 234.0f, header.y, 112.0f, 28.0f},
        .orderFilter = {header.x + header.width - 114.0f, header.y, 112.0f, 28.0f},
        .rows = boundsOf(*root, "rows"),
    };
}

bool pointInUiPanel(Vector2 point, const UiLayout& layout)
{
    return CheckCollisionPointRec(point, layout.topBar)
        || CheckCollisionPointRec(point, computeViewModeBarBounds(layout.worldView))
        || CheckCollisionPointRec(point, layout.leftSidebar)
        || CheckCollisionPointRec(point, layout.rightSidebar)
        || CheckCollisionPointRec(point, layout.bottomPanel);
}

Rectangle computeViewModeBarBounds(Rectangle worldView)
{
    const float horizontalMargin = 32.0f;
    const float availableWidth = std::max(240.0f, worldView.width - horizontalMargin);
    const float width = std::clamp(worldView.width * 0.62f, std::min(520.0f, availableWidth), std::min(760.0f, availableWidth));
    const float height = 38.0f;
    return {worldView.x + (worldView.width - width) * 0.5f, worldView.y + 14.0f, width, height};
}

Rectangle computeViewModeButtonBounds(Rectangle viewModeBar, int index, int count)
{
    if (count <= 0) {
        return viewModeBar;
    }
    const float gap = 4.0f;
    const float padding = 4.0f;
    const float buttonWidth = (viewModeBar.width - padding * 2.0f - gap * static_cast<float>(count - 1)) / static_cast<float>(count);
    return {
        viewModeBar.x + padding + static_cast<float>(index) * (buttonWidth + gap),
        viewModeBar.y + padding,
        buttonWidth,
        viewModeBar.height - padding * 2.0f
    };
}
