#include "ui/panels/MetricsPanel.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace {
void metricRow(const char* icon, const char* label, const char* value, float x, float y, Color valueColor)
{
    IconRegistry::instance().drawIcon(icon, {x, y + 1.0f, 16.0f, 16.0f}, valueColor);
    drawTextClipped(label, {x + 24.0f, y, 112.0f, 18.0f}, 14, {139, 148, 158, 255});
    drawTextClipped(value, {x + 150.0f, y, 82.0f, 18.0f}, 14, valueColor);
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

const MetricContribution* strongestContribution(const MetricsSnapshot& metrics)
{
    const MetricContribution* strongest = nullptr;
    for (const auto& contribution : metrics.contributions) {
        if (strongest == nullptr || std::abs(contribution.amount) > std::abs(strongest->amount)) {
            strongest = &contribution;
        }
    }
    return strongest;
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
}

void MetricsPanel::update(UiContext& context, const Simulation&)
{
    if (context.state == nullptr || !context.state->sandboxMode || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }
    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const LeftSidebarLayout left = computeLeftSidebarLayout(layout.leftSidebar, context.state->sandboxMode);
    const float x = left.sandbox.x;
    const float width = left.sandbox.width;
    const float y = left.sandbox.y;
    if (left.sandbox.height <= 0.0f) {
        return;
    }
    const Vector2 mouse = GetMousePosition();
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

    drawTextClipped("Frontend", {x + 14.0f, y + 158.0f, width - 28.0f, 16.0f}, 12, {139, 148, 158, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.frontend.perceivedLatency * 1000.0);
    metricRow("metric.latency_average", "Perceived Lat.", buffer, x + 14.0f, y + 176.0f, metrics.frontend.perceivedLatency > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f", metrics.frontend.framePressure);
    metricRow("metric.queue_depth", "Frame Pressure", buffer, x + 14.0f, y + 198.0f, scoreColor(metrics.frontend.framePressure, false));
    if (const MetricContribution* contribution = strongestContribution(metrics); contribution != nullptr) {
        std::snprintf(buffer, sizeof(buffer), "%+.1f", contribution->amount);
        metricRow("alert.explanation", contribution->label, buffer, x + 14.0f, y + 220.0f, contribution->amount < 0.0 ? Color{245, 184, 76, 255} : Color{89, 196, 255, 255});
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.0f%%", metrics.frontend.sessionWarmth);
        metricRow("metric.cache_hit_rate", "Session Warmth", buffer, x + 14.0f, y + 220.0f, {151, 111, 255, 255});
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
