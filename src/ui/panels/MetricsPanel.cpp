#include "ui/panels/MetricsPanel.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <cstdio>

namespace {
struct SpecializationSummary {
    MetricsSpecialization id = MetricsSpecialization::Frontend;
    const char* label = "";
    double health = 100.0;
    double trend = 0.0;
    bool implemented = false;
};

void metricRow(const char* icon, const char* label, const char* value, float x, float y, Color valueColor)
{
    IconRegistry::instance().drawIcon(icon, {x, y + 1.0f, 16.0f, 16.0f}, valueColor);
    drawTextClipped(label, {x + 24.0f, y, 112.0f, 18.0f}, 14, {139, 148, 158, 255});
    drawTextClipped(value, {x + 150.0f, y, 82.0f, 18.0f}, 14, valueColor);
}

void compactMetricRow(const char* label, const char* value, float x, float y, float width, Color valueColor)
{
    drawTextClipped(label, {x, y, width - 56.0f, 12.0f}, 11, {139, 148, 158, 255});
    drawTextClipped(value, {x + width - 54.0f, y, 54.0f, 12.0f}, 11, valueColor);
}

double clampScore(double value)
{
    return std::clamp(value, 0.0, 100.0);
}

Color scoreColor(double score, bool higherIsBetter)
{
    const double normalized = higherIsBetter ? score : 100.0 - score;
    if (normalized >= 75.0) {
        return {86, 210, 151, 255};
    }
    if (normalized >= 45.0) {
        return {245, 184, 76, 255};
    }
    return {235, 86, 100, 255};
}

double frontendHealth(const MetricsSnapshot& metrics)
{
    return clampScore(
        100.0
        - metrics.frontend.perceivedLatency * 26.0
        - metrics.frontend.framePressure * 0.18
        - metrics.frontend.sessionStalenessRisk * 0.35
        + metrics.frontend.sessionWarmth * 0.08);
}

const MetricsSnapshot* comparisonSnapshot(const UiState& state)
{
    if (state.metricsHistory.size() < 2) {
        return nullptr;
    }
    return &state.metricsHistory.front();
}

std::array<SpecializationSummary, static_cast<std::size_t>(MetricsSpecialization::Count)> specializationSummaries(
    const MetricsSnapshot& metrics,
    const UiState& state)
{
    double frontendTrend = 0.0;
    if (const MetricsSnapshot* before = comparisonSnapshot(state); before != nullptr) {
        frontendTrend = frontendHealth(metrics) - frontendHealth(*before);
    }
    return {
        SpecializationSummary{MetricsSpecialization::Frontend, "Frontend", frontendHealth(metrics), frontendTrend, true},
        SpecializationSummary{MetricsSpecialization::Backend, "Backend", 100.0 - std::max({metrics.backend.requestLoad, metrics.backend.queuePressure, metrics.backend.computeIntensity}), 0.0, false},
        SpecializationSummary{MetricsSpecialization::Network, "Network", 100.0 - metrics.network.deliveryPressure, 0.0, false},
        SpecializationSummary{MetricsSpecialization::Database, "Database", 100.0 - std::min(100.0, metrics.databaseUtilization * 45.0 + metrics.databaseQueueDepth * 1.5 + metrics.backend.queuePressure * 0.25), 0.0, false},
        SpecializationSummary{MetricsSpecialization::Runtime, "Runtime", 100.0 - metrics.global.complexity * 0.25, 0.0, false},
    };
}

const SpecializationSummary& summaryFor(
    const std::array<SpecializationSummary, static_cast<std::size_t>(MetricsSpecialization::Count)>& summaries,
    MetricsSpecialization specialization)
{
    return summaries[static_cast<std::size_t>(specialization)];
}

const MetricContribution* strongestContribution(const MetricsSnapshot& metrics, MetricContributionDomain domain)
{
    const MetricContribution* strongest = nullptr;
    for (const auto& contribution : metrics.contributions) {
        if (contribution.domain != domain) {
            continue;
        }
        if (strongest == nullptr || std::abs(contribution.amount) > std::abs(strongest->amount)) {
            strongest = &contribution;
        }
    }
    return strongest;
}

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

void drawSpecializationButton(Rectangle bounds, const SpecializationSummary& summary, bool selected)
{
    const Color healthColor = scoreColor(summary.health, true);
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
    compactMetricRow("framePressure", buffer, x, y + 26.0f, width, scoreColor(metrics.frontend.framePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.assetBandwidth);
    compactMetricRow("assetBandwidth", buffer, x, y + 39.0f, width, scoreColor(metrics.frontend.assetBandwidth, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.frontend.interactionDelay * 1000.0);
    compactMetricRow("interactionDelay", buffer, x, y + 52.0f, width, metrics.frontend.interactionDelay > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.websocketPressure);
    compactMetricRow("websocketPressure", buffer, x, y + 65.0f, width, scoreColor(metrics.frontend.websocketPressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.sessionWarmth);
    compactMetricRow("sessionWarmth", buffer, x, y + 78.0f, width, scoreColor(metrics.frontend.sessionWarmth, true));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.sessionStalenessRisk);
    compactMetricRow("sessionStalenessRisk", buffer, x, y + 91.0f, width, scoreColor(metrics.frontend.sessionStalenessRisk, false));
    if (const MetricContribution* contribution = strongestContribution(metrics, MetricContributionDomain::Frontend); contribution != nullptr) {
        std::snprintf(buffer, sizeof(buffer), "%+.1f", contribution->amount);
        compactMetricRow(contribution->label, buffer, x, y + 107.0f, width, contribution->amount < 0.0 ? Color{245, 184, 76, 255} : Color{89, 196, 255, 255});
    }
    EndScissorMode();
}

void drawPendingSpecializationDetails(const SpecializationSummary& summary, Rectangle bounds)
{
    char buffer[32];
    const float x = bounds.x + 14.0f;
    const float y = bounds.y + 100.0f;
    const float width = bounds.width - 28.0f;
    std::snprintf(buffer, sizeof(buffer), "%.0f", summary.health);
    compactMetricRow("health", buffer, x, y, width, scoreColor(summary.health, true));
    compactMetricRow("metrics", "pending", x, y + 13.0f, width, {139, 148, 158, 255});
}
}

void MetricsPanel::update(UiContext& context, const Simulation&)
{
    if (context.state == nullptr || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }
    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const LeftSidebarLayout left = computeLeftSidebarLayout(layout.leftSidebar, context.state->sandboxMode);
    const Vector2 mouse = GetMousePosition();

    if (context.state->showMetrics && left.specializations.height > 0.0f) {
        for (int i = 0; i < static_cast<int>(MetricsSpecialization::Count); ++i) {
            if (CheckCollisionPointRec(mouse, specializationButton(left.specializations, i))) {
                context.state->selectedMetricsSpecialization = static_cast<MetricsSpecialization>(i);
                return;
            }
        }
    }

    if (!context.state->sandboxMode) {
        return;
    }
    const float x = left.sandbox.x;
    const float width = left.sandbox.width;
    const float y = left.sandbox.y;
    if (left.sandbox.height <= 0.0f) {
        return;
    }
    const char* requests[] = {"traffic_spike", "retry_storm", "db_slowdown", "regional_traffic_spike", "recovery"};
    for (int i = 0; i < 5; ++i) {
        if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, i))) {
            context.state->sandboxEventRequest = requests[i];
            return;
        }
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 5))) {
        context.state->sandboxQueueBuildup = !context.state->sandboxQueueBuildup;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 6))) {
        context.state->sandboxTrafficMultiplier = std::max(0.1, context.state->sandboxTrafficMultiplier - 0.25);
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 7))) {
        context.state->sandboxTrafficMultiplier = std::min(8.0, context.state->sandboxTrafficMultiplier + 0.25);
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 8))) {
        context.state->sandboxLatencyMultiplier = std::max(0.1, context.state->sandboxLatencyMultiplier - 0.25);
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 9))) {
        context.state->sandboxLatencyMultiplier = std::min(8.0, context.state->sandboxLatencyMultiplier + 0.25);
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 10))) {
        context.state->sandboxResetSimulationRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 11))) {
        context.state->sandboxClearTimelineRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 12))) {
        context.state->sandboxSlowMotionRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 13))) {
        context.state->sandboxStepRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 14))) {
        context.state->sandboxSeed = std::max(1, context.state->sandboxSeed - 1);
        context.state->sandboxRegenerateRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 15))) {
        context.state->sandboxSeed += 1;
        context.state->sandboxRegenerateRequested = true;
        return;
    }
}

void MetricsPanel::draw(const UiContext& context, const Simulation& simulation) const
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
    metricRow("metric.latency_average", "User Experience", buffer, x + 14.0f, y + 40.0f, scoreColor(metrics.global.userExperience, true));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.infrastructurePressure);
    metricRow("metric.queue_depth", "Infra Pressure", buffer, x + 14.0f, y + 62.0f, scoreColor(metrics.global.infrastructurePressure, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.reliability);
    metricRow("metric.error_rate", "Reliability", buffer, x + 14.0f, y + 84.0f, scoreColor(metrics.global.reliability, true));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.complexity);
    metricRow("metric.complexity", "Complexity", buffer, x + 14.0f, y + 106.0f, scoreColor(metrics.global.complexity, false));
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.global.scalability);
    metricRow("metric.total_traffic", "Scalability", buffer, x + 14.0f, y + 128.0f, scoreColor(metrics.global.scalability, true));

    Rectangle specializations = left.specializations;
    y = specializations.y;
    drawPanelFrame(specializations, "Specializations");
    const auto summaries = specializationSummaries(metrics, *context.state);
    for (int i = 0; i < static_cast<int>(MetricsSpecialization::Count); ++i) {
        const auto specialization = static_cast<MetricsSpecialization>(i);
        drawSpecializationButton(
            specializationButton(specializations, i),
            summaryFor(summaries, specialization),
            specialization == context.state->selectedMetricsSpecialization);
    }
    const SpecializationSummary& selectedSummary = summaryFor(summaries, context.state->selectedMetricsSpecialization);
    std::snprintf(buffer, sizeof(buffer), "%.0f", selectedSummary.health);
    drawTextClipped(selectedSummary.label, {x + 14.0f, y + 94.0f, 112.0f, 12.0f}, 11, {230, 237, 243, 255});
    drawTextClipped(buffer, {x + width - 58.0f, y + 94.0f, 44.0f, 12.0f}, 11, scoreColor(selectedSummary.health, true));
    if (!metrics.activePressureSignals.empty()) {
        const auto& signal = metrics.activePressureSignals.front();
        const std::string label = signal.temporary ? signal.name + " (event)" : signal.name;
        drawTextClipped(label, {x + 92.0f, y + 94.0f, width - 164.0f, 12.0f}, 11, {245, 184, 76, 255});
    }
    if (context.state->selectedMetricsSpecialization == MetricsSpecialization::Frontend) {
        drawFrontendDetails(metrics, specializations);
    } else {
        drawPendingSpecializationDetails(selectedSummary, specializations);
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
