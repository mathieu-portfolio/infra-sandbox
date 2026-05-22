#include "ui/panels/MetricsPanelRenderer.hpp"
#include "ui/panels/LabPanelLayout.hpp"
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


Color withAlpha(Color color, unsigned char alpha)
{
    return {color.r, color.g, color.b, alpha};
}

const char* labActionLabel(lab_panel::Action action, bool queueEnabled)
{
    switch (action) {
    case lab_panel::Action::TrafficSpike: return "Traffic Spike";
    case lab_panel::Action::RetryStorm: return "Retry Storm";
    case lab_panel::Action::DbSlowdown: return "DB Slowdown";
    case lab_panel::Action::RegionalSpike: return "Regional Spike";
    case lab_panel::Action::TrafficDown: return "Traffic -";
    case lab_panel::Action::TrafficUp: return "Traffic +";
    case lab_panel::Action::LatencyDown: return "Latency -";
    case lab_panel::Action::LatencyUp: return "Latency +";
    case lab_panel::Action::Recovery: return "Recovery";
    case lab_panel::Action::QueueToggle: return queueEnabled ? "Queue On" : "Queue Off";
    case lab_panel::Action::ResetSim: return "Reset Sim";
    case lab_panel::Action::ClearLog: return "Clear Log";
    case lab_panel::Action::SlowMo: return "Slow Mo";
    case lab_panel::Action::Step: return "Step";
    case lab_panel::Action::SeedDown: return "Seed -";
    case lab_panel::Action::SeedUp: return "Seed +";
    case lab_panel::Action::Count: return "";
    }
    return "";
}

const char* labActionIcon(lab_panel::Action action)
{
    switch (action) {
    case lab_panel::Action::DbSlowdown: return "lab.database";
    case lab_panel::Action::Recovery: return "lab.recovery";
    case lab_panel::Action::ResetSim: return "alert.warning";
    case lab_panel::Action::ClearLog: return "lab.document";
    case lab_panel::Action::QueueToggle: return "lab.queue";
    case lab_panel::Action::Step: return "lab.play";
    case lab_panel::Action::SeedDown:
    case lab_panel::Action::SeedUp: return "lab.seed";
    case lab_panel::Action::LatencyDown:
    case lab_panel::Action::LatencyUp: return "lab.latency";
    default: return "lab.action";
    }
}

Color labActionColor(lab_panel::Action action, bool queueEnabled)
{
    switch (action) {
    case lab_panel::Action::RetryStorm:
    case lab_panel::Action::ResetSim:
        return {255, 104, 92, 255};
    case lab_panel::Action::DbSlowdown:
    case lab_panel::Action::RegionalSpike:
    case lab_panel::Action::LatencyDown:
    case lab_panel::Action::LatencyUp:
        return {151, 111, 255, 255};
    case lab_panel::Action::Recovery:
        return {86, 210, 151, 255};
    case lab_panel::Action::QueueToggle:
        return queueEnabled ? Color{245, 184, 76, 255} : Color{139, 148, 158, 255};
    default:
        return {89, 196, 255, 255};
    }
}

void drawLabMetricChip(Rectangle bounds, const char* label, const char* value, Color accent)
{
    DrawRectangleRounded(bounds, 0.12f, 6, {18, 23, 30, 235});
    DrawRectangleRoundedLines(bounds, 0.12f, 6, withAlpha(accent, 85));

    constexpr float horizontalPadding = 8.0f;
    constexpr float gap = 8.0f;
    const int valueFontSize = 14;
    const float valueWidth = static_cast<float>(MeasureText(value, valueFontSize));
    const float valueX = bounds.x + bounds.width - horizontalPadding - valueWidth;
    const float labelWidth = std::max(0.0f, valueX - gap - (bounds.x + horizontalPadding));

    drawTextClipped(label, {bounds.x + horizontalPadding, bounds.y + 6.0f, labelWidth, 14.0f}, 12, {139, 148, 158, 255});
    DrawText(value, static_cast<int>(valueX), static_cast<int>(bounds.y + 5.0f), valueFontSize, accent);
}

void drawLabSection(Rectangle bounds, const char* title, const char* icon, Color accent)
{
    DrawRectangleRounded(bounds, 0.06f, 8, {18, 23, 30, 190});
    DrawRectangleRoundedLines(bounds, 0.06f, 8, {70, 86, 104, 90});
    drawIconLabelRow({bounds.x + 10.0f, bounds.y + 6.0f, bounds.width - 20.0f, 20.0f}, icon, title, {
        14.0f,
        6.0f,
        13,
        accent,
        {174, 186, 199, 255},
    });
}

void drawLabButton(Rectangle bounds, lab_panel::Action action, const UiState& state, bool enabled = true)
{
    const Vector2 mouse = GetMousePosition();
    const bool hovered = enabled && CheckCollisionPointRec(mouse, bounds);
    const Color accent = enabled ? labActionColor(action, state.sandboxQueueBuildup) : Color{84, 94, 106, 255};
    const Color fill = hovered ? Color{31, 39, 50, 245} : Color{22, 27, 34, 235};
    const Color border = hovered ? withAlpha(accent, 170) : Color{70, 86, 104, 120};
    DrawRectangleRounded(bounds, 0.12f, 6, fill);
    DrawRectangleRoundedLines(bounds, 0.12f, 6, border);
    drawIconLabelRow({bounds.x + 7.0f, bounds.y + 3.0f, bounds.width - 14.0f, bounds.height - 6.0f},
        labActionIcon(action),
        labActionLabel(action, state.sandboxQueueBuildup),
        {
            14.0f,
            6.0f,
            12,
            accent,
            enabled ? Color{230, 237, 243, 255} : Color{92, 101, 112, 255},
        });
}

void drawLabPanel(Rectangle sandbox, const UiContext& context, const UiFrameView& view)
{
    drawPanelFrame(sandbox, "Infrastructure Lab");
    if (context.state == nullptr) {
        return;
    }

    char buffer[80];
    drawTextClipped("Experiment with load, failures and recovery", {sandbox.x + 14.0f, sandbox.y + 30.0f, sandbox.width - 28.0f, 14.0f}, 12, {139, 148, 158, 255});
    const float chipWidth = (sandbox.width - 36.0f) * 0.5f;
    std::snprintf(buffer, sizeof(buffer), "%.2fx", context.state->sandboxTrafficMultiplier);
    drawLabMetricChip({sandbox.x + 12.0f, sandbox.y + 52.0f, chipWidth, 24.0f}, "Traffic", buffer, {89, 196, 255, 255});
    std::snprintf(buffer, sizeof(buffer), "%.2fx", context.state->sandboxLatencyMultiplier);
    drawLabMetricChip({sandbox.x + 24.0f + chipWidth, sandbox.y + 52.0f, chipWidth, 24.0f}, "Latency", buffer, {151, 111, 255, 255});

    const Rectangle viewBounds = lab_panel::viewport(sandbox);
    const ui::ScissorGuard clip(viewBounds);
    const float scroll = std::clamp(context.state->sandboxScrollOffset, 0.0f, lab_panel::maxScroll(sandbox));
    const Rectangle content = lab_panel::contentBounds(sandbox, scroll);
    const Rectangle traffic = lab_panel::section(content, 0.0f, 154.0f);
    const Rectangle recovery = lab_panel::section(content, 154.0f + lab_panel::sectionGap, 90.0f);
    const Rectangle sim = lab_panel::section(content, 154.0f + lab_panel::sectionGap + 90.0f + lab_panel::sectionGap, 58.0f);
    const Rectangle seed = lab_panel::section(content, 154.0f + lab_panel::sectionGap + 90.0f + lab_panel::sectionGap + 58.0f + lab_panel::sectionGap, 78.0f);

    drawLabSection(traffic, "TRAFFIC & LATENCY", "lab.traffic", {89, 196, 255, 255});
    drawLabSection(recovery, "FAILURES & RECOVERY", "lab.recovery", {86, 210, 151, 255});
    drawLabSection(sim, "SIMULATION", "lab.play", {89, 196, 255, 255});
    drawLabSection(seed, "SEED", "lab.seed", {89, 196, 255, 255});

    for (const auto& button : lab_panel::buttons(sandbox, scroll)) {
        const bool enabled = button.action != lab_panel::Action::SeedDown || context.state->sandboxSeed > 1;
        drawLabButton(button.bounds, button.action, *context.state, enabled);
    }

    const Rectangle seedValue = lab_panel::seedValueBounds(sandbox, scroll);
    DrawRectangleRounded(seedValue, 0.12f, 6, {14, 18, 24, 220});
    DrawRectangleRoundedLines(seedValue, 0.12f, 6, {70, 86, 104, 115});
    std::snprintf(buffer, sizeof(buffer), "Seed %d", context.state->sandboxSeed);
    drawTextClipped(buffer, {seedValue.x + 8.0f, seedValue.y + 6.0f, seedValue.width - 16.0f, 14.0f}, 12, {174, 186, 199, 255});

    std::snprintf(buffer, sizeof(buffer), "Pressure %s | Node %d", pressureCategoryName(view.pressure().dominantPressure), view.pressure().topOverloadedNodeId);
    drawTextClipped(buffer, {seed.x + 10.0f, seed.y + 58.0f, seed.width - 20.0f, 14.0f}, 11, {245, 184, 76, 255});
}


void drawButton(Rectangle bounds, const char* label)
{
    DrawRectangleRounded(bounds, 0.16f, 6, {22, 27, 34, 235});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, {70, 86, 104, 130});
    drawTextClipped(label, {bounds.x + 8.0f, bounds.y + 5.0f, bounds.width - 16.0f, 14.0f}, 12, {230, 237, 243, 255});
}

void metricRow(const char* icon, const char* label, const char* value, float x, float y, Color valueColor)
{
    drawIconLabelRowValue({x, y - 1.0f, 232.0f, 20.0f}, icon, label, value, 82.0f, {
        16.0f,
        8.0f,
        14,
        valueColor,
        {139, 148, 158, 255},
    }, valueColor);
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
            drawIconLabelRow({alertsViewport.x + 14.0f, rowY, alertsViewport.width - 28.0f, 20.0f}, iconId, text, {
                16.0f,
                8.0f,
                13,
                iconColor,
                {230, 237, 243, 255},
            });
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
        drawIconLabelRow({alertsViewport.x + 14.0f, rowY, alertsViewport.width - 28.0f, 20.0f}, "alert.empty_state", "No active incidents", {
            16.0f,
            8.0f,
            14,
            {86, 210, 151, 255},
            {139, 148, 158, 255},
        });
    }
    }

    if (context.state->sandboxMode) {
        drawLabPanel(left.sandbox, context, view);
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
        drawIconLabelRow({x + 14.0f, rowY - 2.0f, width - 28.0f, 20.0f}, icons[static_cast<std::size_t>(i)], names[static_cast<std::size_t>(i)], {
            15.0f,
            9.0f,
            14,
            {89, 196, 255, 255},
            {139, 148, 158, 255},
        });
    }
    }
}
