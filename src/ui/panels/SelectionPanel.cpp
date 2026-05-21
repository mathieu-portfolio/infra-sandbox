#include "ui/panels/SelectionPanel.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"

#include <algorithm>
#include <cstdio>

namespace
{
constexpr float kPanelPadding = 12.0f;
constexpr float kMetricRowSpacing = 22.0f;
constexpr float kSummaryLineHeight = 16.0f;
constexpr int kSummaryFontSize = 13;
constexpr int kSummaryMaxLines = 4;
}

void SelectionPanel::update(UiContext&, const UiFrameView&)
{
}

namespace {
void metricLine(const char* label, const char* value, float x, float y, Color color)
{
    DrawText(label, static_cast<int>(x), static_cast<int>(y), 12, {139, 148, 158, 255});
    drawTextClipped(value, {x + 128.0f, y - 1.0f, 208.0f, 16.0f}, 13, color);
}
}

void SelectionPanel::draw(const UiContext& context, const UiFrameView& view) const
{
    if (context.state == nullptr || context.state->selection.nodeId < 0) {
        return;
    }

    const Node* node = view.graph().node(context.state->selection.nodeId);
    if (node == nullptr) {
        return;
    }

    int incomingTraffic = 0;
    int outgoingTraffic = 0;
    double downstreamWait = 0.0;
    int downstreamCount = 0;
    int dependencyCount = 0;
    double dependencyPressure = 0.0;
    for (const auto& link : view.graph().links()) {
        if (!link.enabled) {
            continue;
        }
        if (link.targetNodeId == node->id) {
            incomingTraffic += static_cast<int>(link.inFlightRequests.size());
            ++dependencyCount;
        }
        if (link.sourceNodeId == node->id) {
            outgoingTraffic += static_cast<int>(link.inFlightRequests.size());
            if (const Node* downstream = view.graph().node(link.targetNodeId)) {
                downstreamWait += downstream->averageQueueWaitSeconds;
                if (const NodePressure* pressure = view.pressureAnalysis().pressureForNode(downstream->id)) {
                    dependencyPressure = std::max(dependencyPressure, pressure->instability);
                }
                ++downstreamCount;
            }
            ++dependencyCount;
        }
    }

    int localRetries = 0;
    for (const auto& [id, request] : view.requests()) {
        (void)id;
        if ((request.currentNodeId == node->id || request.sourceNodeId == node->id) && request.retryCount > 0) {
            ++localRetries;
        }
    }

    const NodePressure* pressure = view.pressureAnalysis().pressureForNode(node->id);
    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const float panelWidth = 364.0f;
    const float contentWidth = panelWidth - kPanelPadding * 2.0f;
    const std::string summary = pressure != nullptr && !pressure->explanation.empty()
        ? pressure->explanation
        : "Inspect adjacent paths to compare local and dependency pressure.";
    const bool hasDependencySummary = pressure != nullptr && !pressure->dependencySummary.empty();
    const bool hasDiagnosis = pressure != nullptr && !pressure->suspectedSource.empty();
    const float summaryHeight = measureTextWrappedHeight(summary, contentWidth, kSummaryFontSize, kSummaryMaxLines);
    const float dependencySummaryHeight = hasDependencySummary
        ? measureTextWrappedHeight(pressure->dependencySummary, contentWidth, kSummaryFontSize, kSummaryMaxLines)
        : 0.0f;
    const float metricsTop = 48.0f;
    const float metricsHeight = kMetricRowSpacing * 8.0f + 30.0f;
    const float textGap = hasDependencySummary ? 10.0f : 0.0f;
    const float panelHeight = metricsTop + metricsHeight + summaryHeight + textGap + dependencySummaryHeight + kPanelPadding;
    const Rectangle panel{layout.worldView.x + 14.0f, layout.worldView.y + 14.0f, panelWidth, panelHeight};
    DrawRectangleRounded(panel, 0.035f, 8, {13, 17, 23, 230});
    DrawRectangleRoundedLines(panel, 0.035f, 8, {89, 196, 255, 120});
    IconRegistry::instance().drawIcon("node.selection_panel", {panel.x + 14.0f, panel.y + 14.0f, 20.0f, 20.0f}, {89, 196, 255, 255});
    drawTextClipped(node->name, {panel.x + 42.0f, panel.y + 13.0f, panel.width - 58.0f, 20.0f}, 16, {230, 237, 243, 255});

    char value[128];
    float y = panel.y + 48.0f;
    std::snprintf(value, sizeof(value), "%d in-flight / %d out", incomingTraffic, outgoingTraffic);
    metricLine("Traffic", value, panel.x + 14.0f, y, {89, 196, 255, 255});
    y += kMetricRowSpacing;
    std::snprintf(value, sizeof(value), "%zu requests", node->queue.size());
    metricLine("Queue Depth", value, panel.x + 14.0f, y, node->queue.empty() ? Color{86, 210, 151, 255} : Color{245, 184, 76, 255});
    y += kMetricRowSpacing;
    std::snprintf(value, sizeof(value), "%.0f%%", node->currentUtilization * 100.0);
    metricLine("Utilization", value, panel.x + 14.0f, y, node->currentUtilization > 0.85 ? Color{235, 86, 100, 255} : Color{230, 237, 243, 255});
    y += kMetricRowSpacing;
    std::snprintf(value, sizeof(value), "%.0f%% saturation", node->stressScore * 100.0);
    metricLine("Saturation", value, panel.x + 14.0f, y, node->stressScore > 0.45 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    y += kMetricRowSpacing;
    std::snprintf(value, sizeof(value), "%.0f%% req success", node->reliabilityScore * 100.0);
    metricLine("Reliability", value, panel.x + 14.0f, y, node->reliabilityScore < 0.9 ? Color{245, 184, 76, 255} : Color{139, 148, 158, 255});
    y += kMetricRowSpacing;
    std::snprintf(value, sizeof(value), "%d local retries", localRetries);
    metricLine("Retries", value, panel.x + 14.0f, y, localRetries > 0 ? Color{235, 86, 100, 255} : Color{139, 148, 158, 255});
    y += kMetricRowSpacing;
    std::snprintf(value, sizeof(value), "%.2fs local / %.2fs down", pressure != nullptr ? pressure->latencyContribution : 0.0, downstreamCount > 0 ? downstreamWait / downstreamCount : 0.0);
    metricLine("Latency Wait", value, panel.x + 14.0f, y, {245, 184, 76, 255});
    y += kMetricRowSpacing;
    const bool hasPrimaryPressure = pressure != nullptr && pressure->dominant != PressureCategory::None;
    metricLine("Local Pressure", hasPrimaryPressure ? pressureCategoryName(pressure->dominant) : "Stable", panel.x + 14.0f, y, hasPrimaryPressure ? Color{151, 111, 255, 255} : Color{86, 210, 151, 255});
    y += kMetricRowSpacing;
    std::snprintf(value, sizeof(value), "%.0f%% across %d links", std::max(dependencyPressure, pressure != nullptr ? pressure->dependencyPressure : 0.0) * 100.0, dependencyCount);
    metricLine("Dependency", value, panel.x + 14.0f, y, dependencyPressure > 0.55 ? Color{245, 184, 76, 255} : Color{139, 148, 158, 255});
    y += 30.0f;

    drawTextWrappedClipped(summary, {panel.x + kPanelPadding, y, contentWidth, summaryHeight}, kSummaryFontSize, {230, 237, 243, 255});
    y += summaryHeight + textGap;
    if (hasDependencySummary) {
        drawTextWrappedClipped(pressure->dependencySummary, {panel.x + kPanelPadding, y, contentWidth, dependencySummaryHeight}, kSummaryFontSize, {139, 148, 158, 255});
    }
}
