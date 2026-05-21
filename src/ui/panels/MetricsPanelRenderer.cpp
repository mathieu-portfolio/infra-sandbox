#include "ui/panels/MetricsPanelRenderer.hpp"
#include "ui/panels/MetricsPanelModel.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"
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
    constexpr float gap = 6.0f;
    constexpr int columns = 3;
    const float buttonWidth = (bounds.width - 28.0f - gap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    return {
        bounds.x + 14.0f + static_cast<float>(index % columns) * (buttonWidth + gap),
        bounds.y + 38.0f + static_cast<float>(index / columns) * 30.0f,
        buttonWidth,
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

void drawSpecializationButton(Rectangle bounds, const metrics_panel::SpecializationSummary& summary, bool selected)
{
    const Color healthColor = metrics_panel::scoreColor(summary.health, true);
    DrawRectangleRounded(bounds, 0.16f, 6, selected ? Color{32, 42, 54, 245} : Color{22, 27, 34, 220});
    DrawRectangleRoundedLines(bounds, 0.16f, 6, selected ? Color{89, 196, 255, 170} : Color{70, 86, 104, 110});
    DrawCircleV({bounds.x + 9.0f, bounds.y + bounds.height * 0.5f}, 3.5f, healthColor);
    const char* trend = summary.trend > 1.0 ? " +" : (summary.trend < -1.0 ? " -" : "");
    char label[32];
    std::snprintf(label, sizeof(label), "%s%s", summary.label, trend);
    drawTextClipped(label, {bounds.x + 17.0f, bounds.y + 5.0f, bounds.width - 20.0f, 14.0f}, 12, summary.implemented ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
}

void drawFrontendDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 100.0f;
    const float width = bounds.width - 28.0f;
    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height));
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.frontend.perceivedLatency * 1000.0);
    compactMetricRow("perceivedLatency", buffer, x, y, width, metrics.frontend.perceivedLatency > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.frontend.renderLatency * 1000.0);
    compactMetricRow("renderLatency", buffer, x, y + 13.0f, width, metrics.frontend.renderLatency > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.framePressure);
    compactMetricRow("framePressure", buffer, x, y + 26.0f, width, metrics_panel::scoreColor(metrics.frontend.framePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.assetBandwidth);
    compactMetricRow("assetBandwidth", buffer, x, y + 39.0f, width, metrics_panel::scoreColor(metrics.frontend.assetBandwidth, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.frontend.interactionDelay * 1000.0);
    compactMetricRow("interactionDelay", buffer, x, y + 52.0f, width, metrics.frontend.interactionDelay > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.websocketPressure);
    compactMetricRow("websocketPressure", buffer, x, y + 65.0f, width, metrics_panel::scoreColor(metrics.frontend.websocketPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.sessionWarmth);
    compactMetricRow("sessionWarmth", buffer, x, y + 78.0f, width, metrics_panel::scoreColor(metrics.frontend.sessionWarmth, true));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.sessionStalenessRisk);
    compactMetricRow("sessionStalenessRisk", buffer, x, y + 91.0f, width, metrics_panel::scoreColor(metrics.frontend.sessionStalenessRisk, false));
    if (const MetricContribution* contribution = metrics_panel::strongestContribution(metrics, MetricContributionDomain::Frontend); contribution != nullptr) {
        std::snprintf(buffer, sizeof(buffer), "%+.1f", contribution->amount);
        compactMetricRow(contribution->label, buffer, x, y + 107.0f, width, contribution->amount < 0.0 ? Color{245, 184, 76, 255} : Color{89, 196, 255, 255});
    }
    EndScissorMode();
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
    const float y = bounds.y + 100.0f;
    const float width = bounds.width - 28.0f;
    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.requestLoad);
    compactMetricRow("requestLoad", buffer, x, y, width, metrics_panel::scoreColor(metrics.backend.requestLoad, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.queuePressure);
    compactMetricRow("queuePressure", buffer, x, y + 13.0f, width, metrics_panel::scoreColor(metrics.backend.queuePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.computeIntensity);
    compactMetricRow("computeIntensity", buffer, x, y + 26.0f, width, metrics_panel::scoreColor(metrics.backend.computeIntensity, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.serviceFragmentation);
    compactMetricRow("serviceFragmentation", buffer, x, y + 39.0f, width, metrics_panel::scoreColor(metrics.backend.serviceFragmentation, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.backend.reliabilityRisk);
    compactMetricRow("reliabilityRisk", buffer, x, y + 52.0f, width, metrics_panel::scoreColor(metrics.backend.reliabilityRisk, false));
    drawContributionRow(metrics, MetricContributionDomain::Backend, x, y + 68.0f, width);
    EndScissorMode();
}

void drawNetworkDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 100.0f;
    const float width = bounds.width - 28.0f;
    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.network.bandwidthPressure);
    compactMetricRow("bandwidthPressure", buffer, x, y, width, metrics_panel::scoreColor(metrics.network.bandwidthPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.network.latencySensitivity);
    compactMetricRow("latencySensitivity", buffer, x, y + 13.0f, width, metrics_panel::scoreColor(metrics.network.latencySensitivity, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.network.trafficBurstiness);
    compactMetricRow("trafficBurstiness", buffer, x, y + 26.0f, width, metrics_panel::scoreColor(metrics.network.trafficBurstiness, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.network.deliveryPressure);
    compactMetricRow("deliveryPressure", buffer, x, y + 39.0f, width, metrics_panel::scoreColor(metrics.network.deliveryPressure, false));
    drawContributionRow(metrics, MetricContributionDomain::Network, x, y + 55.0f, width);
    EndScissorMode();
}

void drawDatabaseDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 100.0f;
    const float width = bounds.width - 28.0f;
    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.readPressure);
    compactMetricRow("readPressure", buffer, x, y, width, metrics_panel::scoreColor(metrics.database.readPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.writePressure);
    compactMetricRow("writePressure", buffer, x, y + 13.0f, width, metrics_panel::scoreColor(metrics.database.writePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.contention);
    compactMetricRow("contention", buffer, x, y + 26.0f, width, metrics_panel::scoreColor(metrics.database.contention, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.replicationLag);
    compactMetricRow("replicationLag", buffer, x, y + 39.0f, width, metrics_panel::scoreColor(metrics.database.replicationLag, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.database.persistenceRisk);
    compactMetricRow("persistenceRisk", buffer, x, y + 52.0f, width, metrics_panel::scoreColor(metrics.database.persistenceRisk, false));
    drawContributionRow(metrics, MetricContributionDomain::Database, x, y + 68.0f, width);
    EndScissorMode();
}

void drawRuntimeDetails(const MetricsSnapshot& metrics, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 100.0f;
    const float width = bounds.width - 28.0f;
    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.cpuPressure);
    compactMetricRow("cpuPressure", buffer, x, y, width, metrics_panel::scoreColor(metrics.runtime.cpuPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.memoryPressure);
    compactMetricRow("memoryPressure", buffer, x, y + 13.0f, width, metrics_panel::scoreColor(metrics.runtime.memoryPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.allocationOrGcPressure);
    compactMetricRow("allocationOrGcPressure", buffer, x, y + 26.0f, width, metrics_panel::scoreColor(metrics.runtime.allocationOrGcPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.schedulingPressure);
    compactMetricRow("schedulingPressure", buffer, x, y + 39.0f, width, metrics_panel::scoreColor(metrics.runtime.schedulingPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.runtime.executionRisk);
    compactMetricRow("executionRisk", buffer, x, y + 52.0f, width, metrics_panel::scoreColor(metrics.runtime.executionRisk, false));
    drawContributionRow(metrics, MetricContributionDomain::Runtime, x, y + 68.0f, width);
    EndScissorMode();
}
}


void MetricsPanelRenderer::draw(const UiContext& context, const Simulation& simulation) const
{
    if (context.state == nullptr || !context.state->showMetrics) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const LeftSidebarLayout left = computeLeftSidebarLayout(layout.leftSidebar, context.state->sandboxMode);
    const float x = layout.leftSidebar.x;
    float y = layout.leftSidebar.y;
    const float width = layout.leftSidebar.width;
    const auto& metrics = simulation.metrics();

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
    const metrics_panel::SpecializationSummary& selectedSummary = metrics_panel::summaryFor(summaries, context.state->selectedMetricsSpecialization);
    std::snprintf(buffer, sizeof(buffer), "%.0f", selectedSummary.health);
    drawTextClipped(selectedSummary.label, {x + 14.0f, y + 94.0f, 112.0f, 12.0f}, 11, {230, 237, 243, 255});
    drawTextClipped(buffer, {x + width - 58.0f, y + 94.0f, 44.0f, 12.0f}, 11, metrics_panel::scoreColor(selectedSummary.health, true));
    if (!metrics.activePressureSignals.empty()) {
        const auto& signal = metrics.activePressureSignals.front();
        const std::string label = signal.temporary ? signal.name + " (event)" : signal.name;
        drawTextClipped(label, {x + 92.0f, y + 94.0f, width - 164.0f, 12.0f}, 11, {245, 184, 76, 255});
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
    BeginScissorMode(static_cast<int>(alerts.x), static_cast<int>(alerts.y), static_cast<int>(alerts.width), static_cast<int>(alerts.height));
    int row = 0;
    for (const auto& hint : simulation.pressure().hints) {
        if (row >= 3) {
            break;
        }
        IconRegistry::instance().drawIcon("alert.hint", {x + 14.0f, y + 42.0f + row * 32.0f, 16.0f, 16.0f}, {245, 184, 76, 255});
        drawTextClipped(hint, {x + 38.0f, y + 40.0f + row * 32.0f, width - 52.0f, 18.0f}, 13, {230, 237, 243, 255});
        ++row;
    }
    for (const auto& pattern : simulation.pressure().suspiciousPatterns) {
        if (row >= 3) {
            break;
        }
        IconRegistry::instance().drawIcon("alert.pattern", {x + 14.0f, y + 42.0f + row * 32.0f, 16.0f, 16.0f}, {151, 111, 255, 255});
        drawTextClipped(pattern, {x + 38.0f, y + 40.0f + row * 32.0f, width - 52.0f, 18.0f}, 13, {230, 237, 243, 255});
        ++row;
    }
    for (const auto& explanation : simulation.pressure().explanations) {
        if (row >= 3) {
            break;
        }
        IconRegistry::instance().drawIcon("alert.explanation", {x + 14.0f, y + 42.0f + row * 32.0f, 16.0f, 16.0f}, {89, 196, 255, 255});
        drawTextClipped(explanation, {x + 38.0f, y + 40.0f + row * 32.0f, width - 52.0f, 18.0f}, 13, {230, 237, 243, 255});
        ++row;
    }
    if (row == 0) {
        IconRegistry::instance().drawIcon("alert.empty_state", {x + 14.0f, y + 42.0f, 16.0f, 16.0f}, {86, 210, 151, 255});
        drawTextClipped("No active incidents", {x + 38.0f, y + 40.0f, width - 52.0f, 18.0f}, 14, {139, 148, 158, 255});
    }
    EndScissorMode();

    if (context.state->sandboxMode) {
        Rectangle sandbox = left.sandbox;
        y = sandbox.y;
        drawPanelFrame(sandbox, "Infrastructure Lab");
        char labBuffer[80];
        std::snprintf(labBuffer, sizeof(labBuffer), "Traffic %.2fx  Latency %.2fx", context.state->sandboxTrafficMultiplier, context.state->sandboxLatencyMultiplier);
        drawTextClipped(labBuffer, {x + 14.0f, y + 22.0f, width - 28.0f, 16.0f}, 12, {139, 148, 158, 255});
        std::snprintf(labBuffer, sizeof(labBuffer), "Pressure %s  Node %d", pressureCategoryName(simulation.pressure().dominantPressure), simulation.pressure().topOverloadedNodeId);
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
    BeginScissorMode(static_cast<int>(legend.x), static_cast<int>(legend.y), static_cast<int>(legend.width), static_cast<int>(legend.height));
    const std::array<const char*, 5> names{"Client Region", "Service", "Database", "Cache", "Network Link"};
    const std::array<const char*, 5> icons{"legend.client", "legend.service", "legend.database", "legend.cache", "legend.link"};
    for (int i = 0; i < 5; ++i) {
        const float rowY = y + 42.0f + static_cast<float>(i) * 24.0f;
        IconRegistry::instance().drawIcon(icons[static_cast<std::size_t>(i)], {x + 14.0f, rowY, 15.0f, 15.0f}, {89, 196, 255, 255});
        drawTextClipped(names[static_cast<std::size_t>(i)], {x + 38.0f, rowY - 1.0f, width - 52.0f, 18.0f}, 14, {139, 148, 158, 255});
    }
    EndScissorMode();
}
