#include "ui/actions/ActionPanelTabs.hpp"

#include "ui/actions/ActionText.hpp"
#include "ui/core/UiCore.hpp"

#include "raylib.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace {
Rectangle nodeBounds(const ui::UiNode& root, const char* id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
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

void drawLockedObservability(Rectangle bounds, NodeInspectionTab tab)
{
    DrawRectangleRounded(bounds, 0.035f, 8, {18, 24, 34, 230});
    DrawRectangleRoundedLines(bounds, 0.035f, 8, {70, 86, 104, 110});
    DrawText("OBSERVABILITY REQUIRED", static_cast<int>(bounds.x + 14.0f), static_cast<int>(bounds.y + 15.0f), 13, {205, 213, 224, 255});
    actions_ui::drawWrappedTextClipped(nodeTabUnlockHint(tab), {bounds.x + 14.0f, bounds.y + 44.0f, bounds.width - 28.0f, bounds.height - 58.0f}, 13, {139, 148, 158, 255});
}
