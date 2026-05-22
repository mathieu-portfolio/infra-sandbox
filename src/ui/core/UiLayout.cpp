#include "ui/core/UiLayout.hpp"

#include "ui/core/UiCore.hpp"

#include <algorithm>

namespace {
Rectangle toRaylib(UiRect rect)
{
    return {rect.x, rect.y, rect.width, rect.height};
}

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

float specializationPanelContentHeight()
{
    constexpr float titleAndTopPadding = 38.0f;
    constexpr float specializationRows = 5.0f * 28.0f;
    constexpr float gapBeforeDetails = 6.0f;
    constexpr float frontendDetailRows = 10.0f * 18.0f;
    constexpr float bottomPadding = 22.0f;
    return titleAndTopPadding + specializationRows + gapBeforeDetails + frontendDetailRows + bottomPadding;
}
}

UiLayout computeUiLayout(int screenWidth, int screenHeight)
{
    DockLayoutState defaultDockState{};
    defaultDockState.leftWidth = std::min(UiTheme::leftWidth, static_cast<float>(screenWidth) * 0.22f);
    defaultDockState.rightWidth = std::min(UiTheme::rightWidth, static_cast<float>(screenWidth) * 0.40f);
    defaultDockState.bottomHeight = std::min(UiTheme::bottomHeight, static_cast<float>(screenHeight) * 0.45f);
    return computeUiLayout(screenWidth, screenHeight, defaultDockState);
}

UiLayout computeUiLayout(int screenWidth, int screenHeight, const DockLayoutState& dockState)
{
    const DockLayoutFrame dock = computeDockLayout(dockState, screenWidth, screenHeight);
    return {
        .topBar = toRaylib(dock.topBar),
        .leftSidebar = toRaylib(dock.leftSidebar),
        .rightSidebar = toRaylib(dock.rightSidebar),
        .bottomPanel = toRaylib(dock.bottomPanel),
        .worldView = toRaylib(dock.worldView),
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
    overview->style(ui::fixedHeight(166.0f));
    root->add(std::move(overview));
    auto specializations = std::make_unique<ui::PanelNode>("specializations");
    specializations->style(ui::contentSize(0.0f, specializationPanelContentHeight()));
    root->add(std::move(specializations));
    auto alerts = std::make_unique<ui::PanelNode>("alerts");
    alerts->style(ui::fixedHeight(132.0f));
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
        .specializations = boundsOf(*root, "specializations"),
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
