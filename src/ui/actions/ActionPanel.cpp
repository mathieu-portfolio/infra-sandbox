#include "ui/actions/ActionPanel.hpp"

#include "core/topology/NodeDefinition.hpp"
#include "ui/actions/ActionPanelModel.hpp"
#include "ui/actions/cards/NodeActionCardView.hpp"
#include "ui/widgets/IconRegistry.hpp"
#include "ui/layout/RightSidebarLayout.hpp"
#include "ui/core/UiCore.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"
#include "ui/actions/ActionFiltering.hpp"
#include "ui/actions/ActionText.hpp"
#include "ui/actions/PressurePresentation.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

Rectangle nodeBounds(const ui::UiNode& root, const char* id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
}

struct ActionHeaderLayout {
    Rectangle title;
    Rectangle capacity;
    Rectangle worldStatus;
    Rectangle filters[4]{};
};

ActionHeaderLayout computeActionHeaderLayout(Rectangle bounds)
{
    auto root = ui::verticalStack("actionHeaderRoot");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = 8.0f;
    root->style(rootStyle);

    auto title = std::make_unique<ui::PanelNode>("title");
    title->style(ui::fixedHeight(18.0f));
    root->add(std::move(title));

    auto capacity = std::make_unique<ui::PanelNode>("capacity");
    capacity->style(ui::fixedHeight(140.0f));
    root->add(std::move(capacity));

    auto filters = ui::grid(4, "filters");
    ui::LayoutStyle filtersStyle;
    filtersStyle.heightMode = ui::SizeMode::Fixed;
    filtersStyle.fixedHeight = 26.0f;
    filtersStyle.widthMode = ui::SizeMode::Flex;
    filtersStyle.flexGrow = 1.0f;
    filters->style(filtersStyle);
    filters->columnGap = 10.0f;
    filters->fixedCellHeight = 26.0f;
    for (int i = 0; i < 4; ++i) {
        auto filter = std::make_unique<ui::PanelNode>("filter" + std::to_string(i));
        filter->style(ui::fixedHeight(26.0f));
        filters->add(std::move(filter));
    }
    root->add(std::move(filters));

    root->measure({bounds.width, bounds.height});
    root->layout(bounds);

    ActionHeaderLayout layout;
    layout.title = nodeBounds(*root, "title");
    layout.capacity = nodeBounds(*root, "capacity");
    layout.worldStatus = {};
    for (int i = 0; i < 4; ++i) {
        layout.filters[i] = nodeBounds(*root, ("filter" + std::to_string(i)).c_str());
    }
    return layout;
}


Rectangle tabBounds(Rectangle bounds, int index, int count)
{
    auto root = ui::grid(std::max(1, count), "tabs");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = 0.0f;
    root->style(rootStyle);
    root->fixedCellHeight = bounds.height;

    for (int i = 0; i < std::max(1, count); ++i) {
        auto tab = std::make_unique<ui::PanelNode>("tab" + std::to_string(i));
        tab->style(ui::fixedHeight(bounds.height));
        root->add(std::move(tab));
    }

    root->measure({bounds.width, bounds.height});
    root->layout(bounds);
    return nodeBounds(*root, ("tab" + std::to_string(std::clamp(index, 0, std::max(1, count) - 1))).c_str());
}

std::string plannedLabel(const UiState& state)
{
    std::string label;
    int shown = 0;
    for (const auto& planned : state.plannedInterventions) {
        if (shown >= 2) {
            break;
        }
        if (!label.empty()) {
            label += ", ";
        }
        label += planned.actionName;
        ++shown;
    }
    if (state.plannedInterventions.size() > static_cast<std::size_t>(shown)) {
        label += ", +" + std::to_string(state.plannedInterventions.size() - static_cast<std::size_t>(shown));
    }
    return label;
}

void drawFilterPill(Rectangle bounds, const std::string& label, bool active)
{
    DrawRectangleRounded(bounds, 0.22f, 8, active ? Color{97, 64, 196, 245} : Color{28, 36, 48, 210});
    DrawRectangleRoundedLines(bounds, 0.22f, 8, active ? Color{145, 109, 255, 210} : Color{70, 86, 104, 90});
    drawTextClipped(label, {bounds.x + 10.0f, bounds.y + 5.0f, bounds.width - 20.0f, 15.0f}, 12, active ? Color{244, 240, 255, 255} : Color{166, 176, 192, 255});
}

void drawMetricBar(Rectangle bounds, const char* label, double value, Color color)
{
    constexpr float kLabelWidth = 56.0f;
    constexpr float kValueWidth = 38.0f;
    const float barX = bounds.x + kLabelWidth + 8.0f;
    const float barWidth = std::max(0.0f, bounds.width - kLabelWidth - kValueWidth - 18.0f);

    DrawText(label, static_cast<int>(bounds.x), static_cast<int>(bounds.y), 13, {185, 195, 210, 255});
    DrawRectangleRounded({barX, bounds.y + 5.0f, barWidth, 5.0f}, 0.5f, 6, {45, 55, 68, 255});
    DrawRectangleRounded({barX, bounds.y + 5.0f, barWidth * static_cast<float>(std::clamp(value, 0.0, 1.0)), 5.0f}, 0.5f, 6, color);
    char text[24];
    std::snprintf(text, sizeof(text), "%.0f%%", value * 100.0);
    DrawText(text, static_cast<int>(bounds.x + bounds.width - kValueWidth), static_cast<int>(bounds.y - 1.0f), 13, {230, 237, 243, 255});
}

struct PressureRow {
    PressureCategory category = PressureCategory::None;
    double value = 0.0;
};

std::vector<PressureRow> primaryPressureRows(const NodePressure* pressure)
{
    std::vector<PressureRow> rows;
    if (pressure == nullptr || pressure->dominant == PressureCategory::None) {
        return rows;
    }

    rows.push_back({pressure->dominant, std::max({pressure->queuePressure, pressure->computePressure, pressure->latencyContribution, pressure->timeoutContribution, pressure->retryContribution, pressure->dependencyPressure, pressure->instability})});

    auto addIfDistinct = [&](PressureCategory category, double value) {
        if (category == PressureCategory::None || value <= 0.08) {
            return;
        }
        for (const auto& row : rows) {
            if (row.category == category) {
                return;
            }
        }
        rows.push_back({category, value});
    };

    addIfDistinct(PressureCategory::QueuePressure, pressure->queuePressure);
    addIfDistinct(PressureCategory::LatencyPressure, pressure->latencyContribution);
    addIfDistinct(PressureCategory::RetryPressure, pressure->retryContribution);
    addIfDistinct(PressureCategory::FailurePressure, pressure->timeoutContribution);
    addIfDistinct(PressureCategory::PersistencePressure, pressure->dependencyPressure);

    if (rows.size() > 3) {
        rows.resize(3);
    }
    return rows;
}

NodeInspectionTab nodeTabAt(int index)
{
    return static_cast<NodeInspectionTab>(std::clamp(index, 0, static_cast<int>(NodeInspectionTab::Count) - 1));
}

const char* nodeTabLabel(NodeInspectionTab tab)
{
    switch (tab) {
    case NodeInspectionTab::Overview: return "Overview";
    case NodeInspectionTab::Metrics: return "Metrics";
    case NodeInspectionTab::Traffic: return "Traffic";
    case NodeInspectionTab::Dependencies: return "Dependencies";
    case NodeInspectionTab::Diagnostics: return "Diagnostics";
    case NodeInspectionTab::Count: break;
    }
    return "Overview";
}

bool nodeTabUnlocked(const ObservabilityState& observability, NodeInspectionTab tab)
{
    switch (tab) {
    case NodeInspectionTab::Overview:
        return true;
    case NodeInspectionTab::Metrics:
        return observability.metricsUnlocked;
    case NodeInspectionTab::Traffic:
        return observability.trafficUnlocked;
    case NodeInspectionTab::Dependencies:
        return observability.dependenciesUnlocked;
    case NodeInspectionTab::Diagnostics:
        return observability.diagnosticsUnlocked;
    case NodeInspectionTab::Count:
        break;
    }
    return true;
}

const char* nodeTabUnlockHint(NodeInspectionTab tab)
{
    switch (tab) {
    case NodeInspectionTab::Metrics:
        return "Metrics are not instrumented yet. Pick an observability world action to expose detailed load, queue, and error readings.";
    case NodeInspectionTab::Traffic:
        return "Traffic analysis is unavailable. Improve observability to inspect request flow, retry traffic, and regional demand.";
    case NodeInspectionTab::Dependencies:
        return "Distributed tracing is unavailable. Improve observability to reveal upstream and downstream dependency paths.";
    case NodeInspectionTab::Diagnostics:
        return "Advanced diagnostics are unavailable. Improve observability to get suspected root causes and confidence estimates.";
    default:
        return "";
    }
}

void drawLockedObservability(Rectangle bounds, NodeInspectionTab tab)
{
    DrawRectangleRounded(bounds, 0.035f, 8, {18, 24, 34, 230});
    DrawRectangleRoundedLines(bounds, 0.035f, 8, {70, 86, 104, 110});
    DrawText("OBSERVABILITY REQUIRED", static_cast<int>(bounds.x + 14.0f), static_cast<int>(bounds.y + 15.0f), 13, {205, 213, 224, 255});
    actions_ui::drawWrappedTextClipped(nodeTabUnlockHint(tab), {bounds.x + 14.0f, bounds.y + 44.0f, bounds.width - 28.0f, bounds.height - 58.0f}, 13, {139, 148, 158, 255});
}


}

void ActionPanel::update(UiContext& context, const Simulation& simulation)
{
    if (context.state == nullptr) {
        return;
    }
    context.state->hoveredActionIndex = -1;
    context.state->hoveredActionEngineeringCosts.clear();
    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);
    const Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
        const RightSidebarLayout panel = computeRightSidebarLayout(layout.rightSidebar);
        constexpr int kNodeTabCount = static_cast<int>(NodeInspectionTab::Count);
        if (CheckCollisionPointRec(mouse, panel.tabs)) {
            for (int i = 0; i < kNodeTabCount; ++i) {
                if (CheckCollisionPointRec(mouse, tabBounds(panel.tabs, i, kNodeTabCount))) {
                    context.state->activeNodeInspectionTab = nodeTabAt(i);
                    return;
                }
            }
        }
    }
    context.state->hoveredWorldActionIndex = -1;
    if (context.state->eventPopupMode != EventPopupMode::None) {
        return;
    }
    if (context.state->gameplayPhase == GameplayPhase::Planning && context.state->worldActionDraftVisible) {
        const Rectangle overlay = WorldActionOverlay::overlayBounds(context.screenWidth, context.screenHeight);
        const int count = static_cast<int>(context.state->worldActionDraft.size());
        for (int i = 0; i < static_cast<int>(context.state->worldActionDraft.size()); ++i) {
            if (CheckCollisionPointRec(mouse, WorldActionOverlay::draftCardBounds(overlay, i, count))) {
                context.state->hoveredWorldActionIndex = i;
                return;
            }
        }
    }
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        if (CheckCollisionPointRec(mouse, cards[static_cast<std::size_t>(i)].bounds)) {
            context.state->hoveredActionIndex = i;
            context.state->hoveredActionEngineeringCosts = cards[static_cast<std::size_t>(i)].engineeringCosts;
            return;
        }
    }
}

void ActionPanel::draw(const UiContext& context, const Simulation& simulation) const
{
    if (context.state == nullptr) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const Rectangle sidebar = layout.rightSidebar;
    const RightSidebarLayout panel = computeRightSidebarLayout(sidebar);
    DrawRectangleRounded(sidebar, 0.018f, 8, {9, 16, 27, 242});
    DrawRectangleRoundedLines(sidebar, 0.018f, 8, {62, 82, 112, 130});

    const Node* selected = simulation.graph().node(context.state->selection.nodeId);
    const NodePressure* selectedPressure = selected != nullptr ? simulation.pressureAnalysis().pressureForNode(selected->id) : nullptr;
    const Rectangle header = panel.header;
    if (selected != nullptr) {
        DrawRectangleRounded({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {37, 50, 82, 255});
        DrawRectangleRoundedLines({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {130, 93, 255, 220});
        IconRegistry::instance().drawIcon("node.header_selected", {header.x + 14.0f, header.y + 18.0f, 26.0f, 26.0f}, {89, 196, 255, 255});
        drawTextClipped(selected->name, {header.x + 70.0f, header.y + 4.0f, header.width - 150.0f, 30.0f}, 25, {241, 245, 249, 255});
        const char* region = selected->hasGeoLocation ? selected->geoLocation.regionName.c_str() : "local";
        drawTextClipped(region, {header.x + 70.0f, header.y + 40.0f, 140.0f, 20.0f}, 14, {166, 176, 192, 255});
        const double load = actions_ui::overallPressure(selectedPressure, selected);
        const char* status = load >= 0.72 ? "Under heavy load" : load >= 0.42 ? "Moderate pressure" : "Stable";
        DrawCircle(static_cast<int>(header.x + header.width - 126.0f), static_cast<int>(header.y + 17.0f), 6.0f, actions_ui::pressureColor(load));
        drawTextClipped(status, {header.x + header.width - 112.0f, header.y + 6.0f, 108.0f, 18.0f}, 14, actions_ui::pressureColor(load));
    } else {
        DrawRectangleRounded({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {37, 50, 82, 255});
        IconRegistry::instance().drawIcon("node.header_empty", {header.x + 14.0f, header.y + 18.0f, 26.0f, 26.0f}, {139, 148, 158, 255});
        drawTextClipped("Select a Node", {header.x + 70.0f, header.y + 6.0f, header.width - 84.0f, 30.0f}, 25, {241, 245, 249, 255});
        drawTextClipped("Inspect local actions and pressures", {header.x + 70.0f, header.y + 42.0f, header.width - 84.0f, 20.0f}, 14, {166, 176, 192, 255});
    }

    DrawLine(static_cast<int>(sidebar.x), static_cast<int>(panel.tabs.y - 5.0f), static_cast<int>(sidebar.x + sidebar.width), static_cast<int>(panel.tabs.y - 5.0f), {31, 42, 58, 255});
    constexpr int kNodeTabCount = static_cast<int>(NodeInspectionTab::Count);
    for (int i = 0; i < kNodeTabCount; ++i) {
        const NodeInspectionTab tabMode = nodeTabAt(i);
        const Rectangle tab = tabBounds(panel.tabs, i, kNodeTabCount);
        const bool active = context.state->activeNodeInspectionTab == tabMode;
        const bool unlocked = nodeTabUnlocked(context.state->observability, tabMode);
        const Color textColor = active ? Color{230, 237, 243, 255} : unlocked ? Color{139, 148, 158, 255} : Color{86, 96, 112, 255};
        drawTextClipped(nodeTabLabel(tabMode), {tab.x + 4.0f, tab.y + 10.0f, tab.width - 8.0f, 18.0f}, 13, textColor);
        if (!unlocked) {
            DrawCircle(static_cast<int>(tab.x + tab.width - 10.0f), static_cast<int>(tab.y + 15.0f), 3.0f, {97, 64, 196, 180});
        }
        if (active) {
            DrawRectangleRounded({tab.x, tab.y + tab.height - 4.0f, tab.width - 10.0f, 3.0f}, 0.4f, 6, {145, 109, 255, 255});
        }
    }

    const Rectangle overview = panel.overview;
    const NodeInspectionTab activeNodeTab = context.state->activeNodeInspectionTab;
    if (!nodeTabUnlocked(context.state->observability, activeNodeTab)) {
        drawLockedObservability(overview, activeNodeTab);
    } else if (activeNodeTab == NodeInspectionTab::Overview) {
        const Rectangle description{overview.x, overview.y, overview.width * 0.58f - 7.0f, overview.height};
        const Rectangle status{description.x + description.width + 14.0f, overview.y, overview.width - description.width - 14.0f, overview.height};
        DrawRectangleRounded(description, 0.035f, 8, {13, 34, 47, 226});
        DrawRectangleRoundedLines(description, 0.035f, 8, {48, 95, 112, 115});
        DrawText("OVERVIEW", static_cast<int>(description.x + 14.0f), static_cast<int>(description.y + 15.0f), 12, {166, 176, 192, 255});
        if (selected != nullptr) {
            const auto& nodeDef = NodeRegistry::definition(selected->type);
            actions_ui::drawWrappedTextClipped(std::string("This ") + std::string(nodeDef.displayName) + " handles local request flow and participates in the architecture.", {description.x + 16.0f, description.y + 44.0f, description.width - 32.0f, 68.0f}, 13, {205, 213, 224, 255});
            const double overall = actions_ui::overallPressure(selectedPressure, selected);
            const char* summary = overall >= 0.72 ? "The node is unstable. Inspect unlocked telemetry before choosing a corrective action."
                : overall >= 0.42 ? "The node is under pressure. More detailed telemetry may clarify whether the cause is local or downstream."
                : "The node is currently healthy. Watch for pressure changes after each operational cycle.";
            actions_ui::drawWrappedTextClipped(summary, {description.x + 16.0f, description.y + 122.0f, description.width - 32.0f, description.height - 134.0f}, 12, {166, 176, 192, 255});
        } else {
            actions_ui::drawWrappedTextClipped("Select an infrastructure node to inspect its role, broad health, and observability-gated details.", {description.x + 16.0f, description.y + 44.0f, description.width - 32.0f, description.height - 58.0f}, 13, {205, 213, 224, 255});
        }

        DrawRectangleRounded(status, 0.035f, 8, {18, 24, 34, 230});
        DrawRectangleRoundedLines(status, 0.035f, 8, {70, 86, 104, 110});
        DrawText("VISIBLE SIGNALS", static_cast<int>(status.x + 14.0f), static_cast<int>(status.y + 15.0f), 14, {205, 213, 224, 255});
        if (selected == nullptr) {
            actions_ui::drawWrappedTextClipped("No node selected. Overview remains intentionally high-level until a concrete node is inspected.", {status.x + 14.0f, status.y + 48.0f, status.width - 28.0f, status.height - 62.0f}, 13, {139, 148, 158, 255});
        } else {
            const double overall = actions_ui::overallPressure(selectedPressure, selected);
            drawTextClipped(actions_ui::pressureSeverity(overall), {status.x + status.width - 78.0f, status.y + 15.0f, 64.0f, 16.0f}, 13, actions_ui::pressureColor(overall));
            drawMetricBar({status.x + 14.0f, status.y + 48.0f, status.width - 28.0f, 18.0f}, "Health", overall, actions_ui::pressureColor(overall));
            DrawText("OBSERVABILITY", static_cast<int>(status.x + 14.0f), static_cast<int>(status.y + 84.0f), 11, {166, 176, 192, 255});
            actions_ui::drawWrappedTextClipped(
                context.state->observability.metricsUnlocked || context.state->observability.dependenciesUnlocked
                    ? "Some telemetry layers are available. Use the tabs to move from broad signals to evidence."
                    : "Only broad status is available. World observability actions unlock metrics, traffic analysis, tracing, and diagnostics.",
                {status.x + 14.0f, status.y + 104.0f, status.width - 28.0f, 62.0f}, 12, {139, 148, 158, 255});
        }
    } else if (activeNodeTab == NodeInspectionTab::Metrics) {
        DrawRectangleRounded(overview, 0.035f, 8, {18, 24, 34, 230});
        DrawRectangleRoundedLines(overview, 0.035f, 8, {70, 86, 104, 110});
        DrawText("METRICS", static_cast<int>(overview.x + 14.0f), static_cast<int>(overview.y + 15.0f), 14, {205, 213, 224, 255});
        if (selected == nullptr) {
            actions_ui::drawWrappedTextClipped("Select a node to inspect instrumented metrics.", {overview.x + 14.0f, overview.y + 48.0f, overview.width - 28.0f, overview.height - 62.0f}, 13, {139, 148, 158, 255});
        } else {
            drawMetricBar({overview.x + 14.0f, overview.y + 48.0f, overview.width - 28.0f, 18.0f}, "Util", selected->currentUtilization, actions_ui::pressureColor(selected->currentUtilization));
            drawMetricBar({overview.x + 14.0f, overview.y + 78.0f, overview.width - 28.0f, 18.0f}, "Queue", selected->queuePressure, actions_ui::pressureColor(selected->queuePressure));
            drawMetricBar({overview.x + 14.0f, overview.y + 108.0f, overview.width - 28.0f, 18.0f}, "Errors", std::max(selected->timeoutPressure, selected->retryPressure), actions_ui::pressureColor(std::max(selected->timeoutPressure, selected->retryPressure)));
            DrawText("PRIMARY PRESSURES", static_cast<int>(overview.x + 14.0f), static_cast<int>(overview.y + 140.0f), 11, {166, 176, 192, 255});
            const auto rows = primaryPressureRows(selectedPressure);
            if (rows.empty()) {
                actions_ui::drawWrappedTextClipped("No primary pressure detected above diagnostic thresholds.", {overview.x + 14.0f, overview.y + 158.0f, overview.width - 28.0f, 32.0f}, 12, {139, 148, 158, 255});
            } else {
                for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
                    const float y = overview.y + 158.0f + static_cast<float>(i) * 16.0f;
                    DrawCircle(static_cast<int>(overview.x + 18.0f), static_cast<int>(y + 6.0f), 4.0f, actions_ui::pressureColor(rows[static_cast<std::size_t>(i)].value));
                    drawTextClipped(pressureCategoryName(rows[static_cast<std::size_t>(i)].category), {overview.x + 28.0f, y, overview.width - 100.0f, 12.0f}, 11, {230, 237, 243, 255});
                    drawTextClipped(actions_ui::pressureSeverity(rows[static_cast<std::size_t>(i)].value), {overview.x + overview.width - 66.0f, y, 52.0f, 12.0f}, 11, actions_ui::pressureColor(rows[static_cast<std::size_t>(i)].value));
                }
            }
        }
    } else if (activeNodeTab == NodeInspectionTab::Traffic) {
        DrawRectangleRounded(overview, 0.035f, 8, {18, 24, 34, 230});
        DrawRectangleRoundedLines(overview, 0.035f, 8, {70, 86, 104, 110});
        DrawText("TRAFFIC ANALYSIS", static_cast<int>(overview.x + 14.0f), static_cast<int>(overview.y + 15.0f), 14, {205, 213, 224, 255});
        if (selected == nullptr) {
            actions_ui::drawWrappedTextClipped("Select a node to inspect traffic flow.", {overview.x + 14.0f, overview.y + 48.0f, overview.width - 28.0f, overview.height - 62.0f}, 13, {139, 148, 158, 255});
        } else {
            int incomingTraffic = 0;
            int outgoingTraffic = 0;
            for (const auto& link : simulation.graph().links()) {
                if (link.targetNodeId == selected->id) incomingTraffic += static_cast<int>(link.inFlightRequests.size());
                if (link.sourceNodeId == selected->id) outgoingTraffic += static_cast<int>(link.inFlightRequests.size());
            }
            char value[96];
            std::snprintf(value, sizeof(value), "In flight: %d incoming / %d outgoing", incomingTraffic, outgoingTraffic);
            drawTextClipped(value, {overview.x + 14.0f, overview.y + 48.0f, overview.width - 28.0f, 18.0f}, 13, {230, 237, 243, 255});
            actions_ui::drawWrappedTextClipped("Traffic analysis separates load movement from local saturation. Use it to distinguish regional demand, retry churn, and downstream congestion.", {overview.x + 14.0f, overview.y + 78.0f, overview.width - 28.0f, 82.0f}, 12, {139, 148, 158, 255});
        }
    } else if (activeNodeTab == NodeInspectionTab::Dependencies) {
        DrawRectangleRounded(overview, 0.035f, 8, {18, 24, 34, 230});
        DrawRectangleRoundedLines(overview, 0.035f, 8, {70, 86, 104, 110});
        DrawText("DEPENDENCIES", static_cast<int>(overview.x + 14.0f), static_cast<int>(overview.y + 15.0f), 14, {205, 213, 224, 255});
        if (selected == nullptr) {
            actions_ui::drawWrappedTextClipped("Select a node to inspect dependency paths.", {overview.x + 14.0f, overview.y + 48.0f, overview.width - 28.0f, overview.height - 62.0f}, 13, {139, 148, 158, 255});
        } else {
            int upstream = 0;
            int downstream = 0;
            for (const auto& link : simulation.graph().links()) {
                if (link.targetNodeId == selected->id) ++upstream;
                if (link.sourceNodeId == selected->id) ++downstream;
            }
            char value[96];
            std::snprintf(value, sizeof(value), "Visible path: %d upstream / %d downstream links", upstream, downstream);
            drawTextClipped(value, {overview.x + 14.0f, overview.y + 48.0f, overview.width - 28.0f, 18.0f}, 13, {230, 237, 243, 255});
            actions_ui::drawWrappedTextClipped(selectedPressure != nullptr && !selectedPressure->explanation.empty()
                    ? selectedPressure->explanation
                    : "Tracing exposes whether a node is the root bottleneck or only suffering from downstream pressure.",
                {overview.x + 14.0f, overview.y + 78.0f, overview.width - 28.0f, 92.0f}, 12, {139, 148, 158, 255});
        }
    } else if (activeNodeTab == NodeInspectionTab::Diagnostics) {
        DrawRectangleRounded(overview, 0.035f, 8, {18, 24, 34, 230});
        DrawRectangleRoundedLines(overview, 0.035f, 8, {70, 86, 104, 110});
        DrawText("DIAGNOSTICS", static_cast<int>(overview.x + 14.0f), static_cast<int>(overview.y + 15.0f), 14, {205, 213, 224, 255});
        if (selected == nullptr) {
            actions_ui::drawWrappedTextClipped("Select a node to inspect suspected root causes.", {overview.x + 14.0f, overview.y + 48.0f, overview.width - 28.0f, overview.height - 62.0f}, 13, {139, 148, 158, 255});
        } else {
            const double confidenceValue = selectedPressure != nullptr ? selectedPressure->diagnosisConfidence : 0.0;
            const char* confidence = confidenceValue >= 0.70 ? "Confidence: high" : confidenceValue >= 0.45 ? "Confidence: medium" : "Confidence: low";
            drawTextClipped(confidence, {overview.x + 14.0f, overview.y + 48.0f, overview.width - 28.0f, 18.0f}, 13, {230, 237, 243, 255});
            std::string diagnosticText = "No strong root cause detected. Diagnostics should support reasoning, not replace it.";
            if (selectedPressure != nullptr && !selectedPressure->suspectedSource.empty()) {
                diagnosticText = "Suspected source: " + selectedPressure->suspectedSource;
                if (!selectedPressure->pressureChain.empty()) {
                    diagnosticText += "\nChain: " + selectedPressure->pressureChain;
                }
            }
            actions_ui::drawWrappedTextClipped(diagnosticText,
                {overview.x + 14.0f, overview.y + 78.0f, overview.width - 28.0f, 92.0f}, 12, {139, 148, 158, 255});
        }
    }

    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);
    const ActionSectionsLayout actions = model.actionSectionsLayout(*context.state, context.screenWidth, context.screenHeight);
    DrawText("AVAILABLE NODE ACTIONS", static_cast<int>(actions.title.x), static_cast<int>(actions.title.y), 14, {230, 237, 243, 255});
    engineeringCapacityPanel_.draw(actions.capacity, *context.state);
    for (int i = 0; i < 4; ++i) {
        drawFilterPill(actions.filters[i], actions_ui::categoryFilterLabel(cards, i), i == 0);
    }
    BeginScissorMode(static_cast<int>(actions.actionList.x - 2.0f), static_cast<int>(actions.actionList.y), static_cast<int>(actions.actionList.width + 4.0f), static_cast<int>(actions.actionList.height));
    if (cards.empty()) {
        drawTextClipped("Select an API, database, cache, or queue node to see contextual actions.", {actions.actionList.x, actions.actionList.y + 4.0f, actions.actionList.width, 18.0f}, 13, {139, 148, 158, 255});
    }
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        const auto& card = cards[static_cast<std::size_t>(i)];
        if (!card.available || card.bounds.height <= 0.0f) {
            continue;
        }
        const bool highlighted = i == context.state->hoveredActionIndex || i == context.state->selectedActionIndex;
        NodeActionCardView{}.draw(card, highlighted);
    }
    EndScissorMode();

    const Rectangle locked = panel.locked;
    DrawRectangleRounded(locked, 0.045f, 8, {17, 24, 34, 230});
    DrawRectangleRoundedLines(locked, 0.045f, 8, {70, 86, 104, 110});
    IconRegistry::instance().drawIcon("action.locked", {locked.x + 16.0f, locked.y + 24.0f, 24.0f, 24.0f}, {139, 148, 158, 180});
    int lockedCount = 0;
    std::string lockedReason = "More actions will be discovered as scenarios unlock concepts.";
    for (const auto& card : cards) {
        if (!card.available) {
            ++lockedCount;
            if (!card.unavailableReason.empty()) {
                lockedReason = card.unavailableReason;
            }
        }
    }
    drawTextClipped(lockedCount > 0 ? std::to_string(lockedCount) + " locked action(s)" : "Locked actions", {locked.x + 56.0f, locked.y + 17.0f, locked.width - 72.0f, 20.0f}, 13, {166, 176, 192, 255});
    drawTextClipped(lockedReason, {locked.x + 56.0f, locked.y + 42.0f, locked.width - 72.0f, 18.0f}, 12, {139, 148, 158, 255});

    if (context.state->placementActive) {
        drawTextClipped("Placement: hover the map, then click a region.", panel.statusMessage, 12, {89, 196, 255, 255});
    }

    DrawRectangleRounded(panel.button, 0.08f, 8, {97, 64, 196, static_cast<unsigned char>(context.state->placementActive ? 120 : 150)});
    const bool hasSelectedAction = context.state->selectedActionIndex >= 0;
    DrawText(context.state->placementActive ? "Pick on Map" : (hasSelectedAction ? "Queue Action" : "Select an Action"), static_cast<int>(panel.button.x + panel.button.width * 0.5f - 58.0f), static_cast<int>(panel.button.y + 14.0f), 15, {230, 237, 243, 255});

    if (!context.state->latestFeedback.empty() && !context.state->placementActive) {
        drawTextClipped(context.state->latestFeedback, panel.statusMessage, 13, {245, 184, 76, 255});
    }
    if (!context.state->plannedInterventions.empty()) {
        drawTextClipped("Planned: " + plannedLabel(*context.state), panel.summaryMessage, 12, {86, 210, 151, 255});
    } else if (context.state->selectedWorldActionIndex >= 0 && context.state->selectedWorldActionIndex < static_cast<int>(context.state->worldActionDraft.size())) {
        drawTextClipped("World action: " + context.state->worldActionDraft[static_cast<std::size_t>(context.state->selectedWorldActionIndex)].name, panel.summaryMessage, 12, {86, 210, 151, 255});
    } else if (!context.state->resolutionSummaries.empty()) {
        drawTextClipped(context.state->resolutionSummaries.front(), panel.summaryMessage, 12, {86, 210, 151, 255});
    }
}

void ActionPanel::drawPlanningOverlays(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    eventOverlay_.draw(context, scenarioManager);
    worldActionOverlay_.draw(context);
}
