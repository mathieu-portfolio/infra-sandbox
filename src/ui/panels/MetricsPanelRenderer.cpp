#include "ui/panels/MetricsPanelRenderer.hpp"
#include "ui/panels/MetricsPanelModel.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"
#include "ui/core/ScrollHandling.hpp"
#include "ui/actions/PressurePresentation.hpp"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <string>

namespace {
Rectangle specializationButton(Rectangle bounds, int index)
{
    constexpr float rowHeight = 28.0f;
    return {
        bounds.x + 14.0f,
        bounds.y + 38.0f + static_cast<float>(index) * rowHeight,
        bounds.width - 28.0f,
        24.0f,
    };
}

Rectangle sandboxButton(float x, float y, float width, int index)
{
    return {x + 12.0f + static_cast<float>(index % 2) * ((width - 30.0f) * 0.5f + 6.0f), y + 42.0f + static_cast<float>(index / 2) * 30.0f, (width - 30.0f) * 0.5f, 24.0f};
}

void drawButton(Rectangle bounds, const char* label)
{
    DrawRectangleRounded(bounds, 0.16f, 6, {22, 27, 34, 235});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, {70, 86, 104, 130});
    drawTextClipped(label, {bounds.x + 8.0f, bounds.y + 5.0f, bounds.width - 16.0f, 14.0f}, 12, {230, 237, 243, 255});
}

void metricRow(const char* icon, const char* label, const char* value, float x, float y, Color valueColor)
{
    IconRegistry::instance().drawIcon(icon, {x, y + 1.0f, 16.0f, 16.0f}, valueColor);
    drawTextClipped(label, {x + 24.0f, y, 112.0f, 18.0f}, 14, {139, 148, 158, 255});
    drawTextClipped(value, {x + 150.0f, y, 82.0f, 18.0f}, 14, valueColor);
}

void compactMetricRow(const char* label, const char* value, float x, float y, float width, Color valueColor)
{
    drawTextClipped(label, {x, y, width - 72.0f, 15.0f}, 12, {139, 148, 158, 255});
    drawTextClipped(value, {x + width - 70.0f, y, 70.0f, 15.0f}, 12, valueColor);
}

void drawSpecializationButton(Rectangle bounds, const metrics_panel::SpecializationSummary& summary, bool selected)
{
    const Color healthColor = metrics_panel::scoreColor(summary.health, true);
    DrawRectangleRounded(bounds, 0.16f, 6, selected ? Color{32, 42, 54, 245} : Color{22, 27, 34, 220});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, selected ? Color{89, 196, 255, 170} : Color{70, 86, 104, 110});
    DrawCircleV({bounds.x + 10.0f, bounds.y + bounds.height * 0.5f}, 3.5f, healthColor);
    drawTextClipped(summary.label, {bounds.x + 22.0f, bounds.y + 5.0f, bounds.width - 30.0f, 14.0f}, 12, summary.implemented ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
}

void drawFrontendDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 184.0f;
    const float width = bounds.width - 28.0f;
    {
    const ui::ScissorGuard clip(bounds);
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.frontend.perceivedLatency * 1000.0);
    compactMetricRow("Perceived latency", buffer, x, y, width, metrics.frontend.perceivedLatency > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.frontend.renderLatency * 1000.0);
    compactMetricRow("Render latency", buffer, x, y + 18.0f, width, metrics.frontend.renderLatency > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.framePressure);
    compactMetricRow("Frame pressure", buffer, x, y + 36.0f, width, metrics_panel::scoreColor(metrics.frontend.framePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.assetBandwidth);
    compactMetricRow("Asset bandwidth", buffer, x, y + 54.0f, width, metrics_panel::scoreColor(metrics.frontend.assetBandwidth, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.frontend.interactionDelay * 1000.0);
    compactMetricRow("Interaction delay", buffer, x, y + 72.0f, width, metrics.frontend.interactionDelay > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.websocketPressure);
    compactMetricRow("WebSocket pressure", buffer, x, y + 90.0f, width, metrics_panel::scoreColor(metrics.frontend.websocketPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.sessionWarmth);
    compactMetricRow("Session warmth", buffer, x, y + 108.0f, width, metrics_panel::scoreColor(metrics.frontend.sessionWarmth, true));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.sessionStalenessRisk);
    compactMetricRow("Staleness risk", buffer, x, y + 126.0f, width, metrics_panel::scoreColor(metrics.frontend.sessionStalenessRisk, false));
    if (const MetricContribution* contribution = metrics_panel::strongestContribution(metrics, MetricContributionDomain::Frontend); contribution != nullptr) {
        std::snprintf(buffer, sizeof(buffer), "%+.1f", contribution->amount);
        compactMetricRow(contribution->label, buffer, x, y + 148.0f, width, contribution->amount < 0.0 ? Color{245, 184, 76, 255} : Color{89, 196, 255, 255});
    }
    }
}

void drawContributionRow(const MetricsSnapshot& metrics, MetricContributionDomain domain, float x, float y, float width)
{
    char buffer[32];
    if (const MetricContribution* contribution = metrics_panel::strongestContribution(metrics, domain); contribution != nullptr) {
        std::snprintf(buffer, sizeof(buffer), "%+.1f", contribution->amount);
        compactMetricRow(contribution->label, buffer, x, y, width, contribution->amount < 0.0 ? Color{245, 184, 76, 255} : Color{89, 196, 255, 255});
    }
}

void drawBackendDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 184.0f;
    const float width = bounds.width - 28.0f;
    {
    const ui::ScissorGuard clip(bounds);
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.requestLoad);
    compactMetricRow("Request load", buffer, x, y, width, metrics_panel::scoreColor(metrics.backend.requestLoad, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.queuePressure);
    compactMetricRow("Queue pressure", buffer, x, y + 18.0f, width, metrics_panel::scoreColor(metrics.backend.queuePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.computeIntensity);
    compactMetricRow("Compute intensity", buffer, x, y + 36.0f, width, metrics_panel::scoreColor(metrics.backend.computeIntensity, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.serviceFragmentation);
    compactMetricRow("Service fragmentation", buffer, x, y + 54.0f, width, metrics_panel::scoreColor(metrics.backend.serviceFragmentation, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.reliabilityRisk);
    compactMetricRow("Reliability risk", buffer, x, y + 72.0f, width, metrics_panel::scoreColor(metrics.backend.reliabilityRisk, false));
    drawContributionRow(metrics, MetricContributionDomain::Backend, x, y + 94.0f, width);
    }
}

void drawNetworkDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 184.0f;
    const float width = bounds.width - 28.0f;
    {
    const ui::ScissorGuard clip(bounds);
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.network.bandwidthPressure);
    compactMetricRow("Bandwidth pressure", buffer, x, y, width, metrics_panel::scoreColor(metrics.network.bandwidthPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.network.latencySensitivity);
    compactMetricRow("Latency sensitivity", buffer, x, y + 18.0f, width, metrics_panel::scoreColor(metrics.network.latencySensitivity, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.network.trafficBurstiness);
    compactMetricRow("Traffic burstiness", buffer, x, y + 36.0f, width, metrics_panel::scoreColor(metrics.network.trafficBurstiness, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.network.deliveryPressure);
    compactMetricRow("Delivery pressure", buffer, x, y + 54.0f, width, metrics_panel::scoreColor(metrics.network.deliveryPressure, false));
    drawContributionRow(metrics, MetricContributionDomain::Network, x, y + 76.0f, width);
    }
}

void drawDatabaseDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 184.0f;
    const float width = bounds.width - 28.0f;
    {
    const ui::ScissorGuard clip(bounds);
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.readPressure);
    compactMetricRow("Read pressure", buffer, x, y, width, metrics_panel::scoreColor(metrics.database.readPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.writePressure);
    compactMetricRow("Write pressure", buffer, x, y + 18.0f, width, metrics_panel::scoreColor(metrics.database.writePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.contention);
    compactMetricRow("Contention", buffer, x, y + 36.0f, width, metrics_panel::scoreColor(metrics.database.contention, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.replicationLag);
    compactMetricRow("Replication lag", buffer, x, y + 54.0f, width, metrics_panel::scoreColor(metrics.database.replicationLag, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.persistenceRisk);
    compactMetricRow("Persistence risk", buffer, x, y + 72.0f, width, metrics_panel::scoreColor(metrics.database.persistenceRisk, false));
    drawContributionRow(metrics, MetricContributionDomain::Database, x, y + 94.0f, width);
    }
}

void drawRuntimeDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 184.0f;
    const float width = bounds.width - 28.0f;
    {
    const ui::ScissorGuard clip(bounds);
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.cpuPressure);
    compactMetricRow("CPU pressure", buffer, x, y, width, metrics_panel::scoreColor(metrics.runtime.cpuPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.memoryPressure);
    compactMetricRow("Memory pressure", buffer, x, y + 18.0f, width, metrics_panel::scoreColor(metrics.runtime.memoryPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.allocationOrGcPressure);
    compactMetricRow("Alloc / GC pressure", buffer, x, y + 36.0f, width, metrics_panel::scoreColor(metrics.runtime.allocationOrGcPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.schedulingPressure);
    compactMetricRow("Scheduling pressure", buffer, x, y + 54.0f, width, metrics_panel::scoreColor(metrics.runtime.schedulingPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.executionRisk);
    compactMetricRow("Execution risk", buffer, x, y + 72.0f, width, metrics_panel::scoreColor(metrics.runtime.executionRisk, false));
    drawContributionRow(metrics, MetricContributionDomain::Runtime, x, y + 94.0f, width);
    }
}
}


void MetricsPanelRenderer::draw(const UiContext& context, const UiFrameView& view) const
{
    if (context.state == nullptr || !context.state->showMetrics) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight, context.state->dockLayout);
    const LeftSidebarLayout left = computeLeftSidebarLayout(layout.leftSidebar, context.state->sandboxMode);
    const float x = layout.leftSidebar.x;
    float y = layout.leftSidebar.y;
    const float width = layout.leftSidebar.width;
    const auto& metrics = view.metrics();

    Rectangle overview = left.overview;
    y = overview.y;
    drawPanelFrame(overview, "System Overview");
    char buffer[80];
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.userExperience);
    metricRow("metric.latency_average", "User Experience", buffer, x + 14.0f, y + 40.0f, metrics_panel::scoreColor(metrics.global.userExperience, true));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.infrastructurePressure);
    metricRow("metric.queue_depth", "Infra Pressure", buffer, x + 14.0f, y + 62.0f, metrics_panel::scoreColor(metrics.global.infrastructurePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.reliability);
    metricRow("metric.error_rate", "Reliability", buffer, x + 14.0f, y + 84.0f, metrics_panel::scoreColor(metrics.global.reliability, true));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.complexity);
    metricRow("metric.complexity", "Complexity", buffer, x + 14.0f, y + 106.0f, metrics_panel::scoreColor(metrics.global.complexity, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.scalability);
    metricRow("metric.total_traffic", "Scalability", buffer, x + 14.0f, y + 128.0f, metrics_panel::scoreColor(metrics.global.scalability, true));

    Rectangle specializations = left.specializations;
    y = specializations.y;
    drawPanelFrame(specializations, "Specializations");
    const auto summaries = metrics_panel::specializationSummaries(metrics, *context.state);
    for (int i = 0; i < static_cast<int>(MetricsSpecialization::Count); ++i) {
        const auto specialization = static_cast<MetricsSpecialization>(i);
        drawSpecializationButton(
            specializationButton(specializations, i),
            metrics_panel::summaryFor(summaries, specialization),
            specialization == context.state->selectedMetricsSpecialization);
    }
    if (!metrics.activePressureSignals.empty()) {
        const auto& signal = metrics.activePressureSignals.front();
        const std::string label = signal.temporary ? signal.name + " (event)" : signal.name;
        drawTextClipped(label, {x + 14.0f, specializations.y + specializations.height - 18.0f, width - 28.0f, 14.0f}, 11, {245, 184, 76, 255});
    }
    switch (context.state->selectedMetricsSpecialization) {
    case MetricsSpecialization::Frontend:
        drawFrontendDetails(metrics, specializations);
        break;
    case MetricsSpecialization::Backend:
        drawBackendDetails(metrics, specializations);
        break;
    case MetricsSpecialization::Network:
        drawNetworkDetails(metrics, specializations);
        break;
    case MetricsSpecialization::Database:
        drawDatabaseDetails(metrics, specializations);
        break;
    case MetricsSpecialization::Runtime:
        drawRuntimeDetails(metrics, specializations);
        break;
    case MetricsSpecialization::Count:
        break;
    }

    Rectangle alerts = left.alerts;
    y = alerts.y;
    drawPanelFrame(alerts, "Alerts");
    {
    constexpr float alertsHeaderHeight = 32.0f;
    constexpr float alertsTopPadding = 8.0f;
    constexpr float alertsRowHeight = 32.0f;
    const Rectangle alertsViewport = ui::scrollViewport(alerts, alertsHeaderHeight);
    const ui::ScissorGuard alertsClip(alertsViewport);
    int row = 0;
    const float scrollY = context.state != nullptr ? context.state->alertsScrollOffset : 0.0f;
    auto drawAlertRow = [&](const char* iconId, const std::string& text, Color iconColor) {
        const float rowY = alertsViewport.y + alertsTopPadding + static_cast<float>(row) * alertsRowHeight - scrollY;
        if (rowY > alertsViewport.y - alertsRowHeight && rowY < alertsViewport.y + alertsViewport.height) {
            IconRegistry::instance().drawIcon(iconId, {alertsViewport.x + 14.0f, rowY + 2.0f, 16.0f, 16.0f}, iconColor);
            drawTextClipped(text, {alertsViewport.x + 38.0f, rowY, alertsViewport.width - 52.0f, 18.0f}, 13, {230, 237, 243, 255});
        }
        ++row;
    };
    for (const auto& hint : view.pressure().hints) {
        drawAlertRow("alert.hint", hint, {245, 184, 76, 255});
    }
    for (const auto& pattern : view.pressure().suspiciousPatterns) {
        drawAlertRow("alert.pattern", pattern, {151, 111, 255, 255});
    }
    for (const auto& explanation : view.pressure().explanations) {
        drawAlertRow("alert.explanation", explanation, {89, 196, 255, 255});
    }
    if (row == 0) {
        const float rowY = alertsViewport.y + alertsTopPadding - scrollY;
        IconRegistry::instance().drawIcon("alert.empty_state", {alertsViewport.x + 14.0f, rowY + 2.0f, 16.0f, 16.0f}, {86, 210, 151, 255});
        drawTextClipped("No active incidents", {alertsViewport.x + 38.0f, rowY, alertsViewport.width - 52.0f, 18.0f}, 14, {139, 148, 158, 255});
    }
    }

    if (context.state->sandboxMode) {
        Rectangle sandbox = left.sandbox;
        y = sandbox.y;
        drawPanelFrame(sandbox, "Infrastructure Lab");
        char labBuffer[80];
        std::snprintf(labBuffer, sizeof(labBuffer), "Traffic %.2fx  Latency %.2fx", context.state->sandboxTrafficMultiplier, context.state->sandboxLatencyMultiplier);
        drawTextClipped(labBuffer, {x + 14.0f, y + 22.0f, width - 28.0f, 16.0f}, 12, {139, 148, 158, 255});
        std::snprintf(labBuffer, sizeof(labBuffer), "Pressure %s  Node %d", pressureCategoryName(view.pressure().dominantPressure), view.pressure().topOverloadedNodeId);
        drawTextClipped(labBuffer, {x + 14.0f, y + 256.0f, width - 28.0f, 14.0f}, 12, {245, 184, 76, 255});
        const char* labels[] = {
            "Traffic Spike", "Retry Storm", "DB Slowdown", "Regional Spike", "Recovery", context.state->sandboxQueueBuildup ? "Queue On" : "Queue Off",
            "Traffic -", "Traffic +", "Latency -", "Latency +", "Reset Sim", "Clear Log", "Slow Mo", "Step", "Seed -", "Seed +"
        };
        for (int i = 0; i < 16; ++i) {
            drawButton(sandboxButton(x, y, width, i), labels[i]);
        }
        std::snprintf(labBuffer, sizeof(labBuffer), "Seed %d", context.state->sandboxSeed);
        drawTextClipped(labBuffer, {x + 14.0f, y + 274.0f, width - 28.0f, 14.0f}, 12, {89, 196, 255, 255});
    }

    Rectangle legend = left.legend;
    y = legend.y;
    drawPanelFrame(legend, "Legend");
    {
    const ui::ScissorGuard legendClip(legend);
    const std::array<const char*, 5> names{"Client Region", "Service", "Database", "Cache", "Network Link"};
    const std::array<const char*, 5> icons{"legend.client", "legend.service", "legend.database", "legend.cache", "legend.link"};
    for (int i = 0; i < 5; ++i) {
        const float rowY = y + 42.0f + static_cast<float>(i) * 24.0f;
        IconRegistry::instance().drawIcon(icons[static_cast<std::size_t>(i)], {x + 14.0f, rowY, 15.0f, 15.0f}, {89, 196, 255, 255});
        drawTextClipped(names[static_cast<std::size_t>(i)], {x + 38.0f, rowY - 1.0f, width - 52.0f, 18.0f}, 14, {139, 148, 158, 255});
    }
    }
}
