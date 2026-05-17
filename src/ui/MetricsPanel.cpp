#include "ui/MetricsPanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"
#include "ui/UiPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace {
void metricRow(const char* icon, const char* label, const char* value, float x, float y, Color valueColor)
{
    IconRegistry::instance().drawIcon(icon, {x, y + 1.0f, 16.0f, 16.0f}, valueColor);
    drawTextClipped(label, {x + 24.0f, y, 112.0f, 18.0f}, 14, {139, 148, 158, 255});
    drawTextClipped(value, {x + 150.0f, y, 82.0f, 18.0f}, 14, valueColor);
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
    const float x = layout.leftSidebar.x;
    const float width = layout.leftSidebar.width;
    const float y = layout.leftSidebar.y + 196.0f + UiTheme::gap + 166.0f + UiTheme::gap;
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
    const float x = layout.leftSidebar.x;
    float y = layout.leftSidebar.y;
    const float width = layout.leftSidebar.width;
    const auto& metrics = simulation.metrics();

    Rectangle overview{x, y, width, 226.0f};
    drawPanelFrame(overview, "System Overview");
    char buffer[80];
    std::snprintf(buffer, sizeof(buffer), "%.1f req/s", metrics.inputRatePerSecond);
    metricRow("metric.traffic", "Total Traffic", buffer, x + 14.0f, y + 42.0f, {89, 196, 255, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f ms", metrics.averageLatencySeconds * 1000.0);
    metricRow("metric.latency", "Avg Latency", buffer, x + 14.0f, y + 72.0f, metrics.averageLatencySeconds > 1.0 ? Color{245, 184, 76, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%.1f/s", metrics.timeoutRatePerSecond);
    metricRow("metric.errors", "Error Rate", buffer, x + 14.0f, y + 102.0f, metrics.timeoutRatePerSecond > 0.5 ? Color{235, 86, 100, 255} : Color{86, 210, 151, 255});
    std::snprintf(buffer, sizeof(buffer), "%d / %d", metrics.apiQueueDepth, metrics.databaseQueueDepth);
    metricRow("metric.queue", "API / DB Queue", buffer, x + 14.0f, y + 132.0f, {245, 184, 76, 255});
    std::snprintf(buffer, sizeof(buffer), "%.0f%%", metrics.cacheHitRate * 100.0);
    metricRow("node.cache", "Cache Hit Rate", buffer, x + 14.0f, y + 162.0f, {151, 111, 255, 255});
    std::snprintf(buffer, sizeof(buffer), "%.1f / %.0f", metrics.complexity.current, metrics.complexity.recommendedThreshold);
    metricRow("layer.complexity", "Complexity", buffer, x + 14.0f, y + 192.0f, metrics.complexity.current > metrics.complexity.recommendedThreshold ? Color{245, 184, 76, 255} : Color{210, 168, 255, 255});

    y += overview.height + UiTheme::gap;
    Rectangle alerts{x, y, width, 166.0f};
    drawPanelFrame(alerts, "Alerts");
    BeginScissorMode(static_cast<int>(alerts.x), static_cast<int>(alerts.y), static_cast<int>(alerts.width), static_cast<int>(alerts.height));
    int row = 0;
    for (const auto& hint : simulation.pressure().hints) {
        if (row >= 3) {
            break;
        }
        IconRegistry::instance().drawIcon("alert.warning", {x + 14.0f, y + 42.0f + row * 32.0f, 16.0f, 16.0f}, {245, 184, 76, 255});
        drawTextClipped(hint, {x + 38.0f, y + 40.0f + row * 32.0f, width - 52.0f, 18.0f}, 13, {230, 237, 243, 255});
        ++row;
    }
    if (row == 0) {
        IconRegistry::instance().drawIcon("alert.ok", {x + 14.0f, y + 42.0f, 16.0f, 16.0f}, {86, 210, 151, 255});
        drawTextClipped("No active incidents", {x + 38.0f, y + 40.0f, width - 52.0f, 18.0f}, 14, {139, 148, 158, 255});
    }
    EndScissorMode();

    y += alerts.height + UiTheme::gap;
    if (context.state->sandboxMode) {
        Rectangle sandbox{x, y, width, 288.0f};
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
        y += sandbox.height + UiTheme::gap;
    }

    if (y + 178.0f > layout.leftSidebar.y + layout.leftSidebar.height) {
        return;
    }
    Rectangle layers{x, y, width, 178.0f};
    drawPanelFrame(layers, "Layers");
    const std::array<const char*, 5> layerNames{"Traffic Flow", "Resources", "Persistence", "Reliability", "Geography"};
    const std::array<const char*, 5> layerIcons{"layer.flow", "layer.resources", "layer.persistence", "layer.reliability", "layer.geography"};
    for (int i = 0; i < 5; ++i) {
        const float rowY = y + 42.0f + static_cast<float>(i) * 26.0f;
        IconRegistry::instance().drawIcon(layerIcons[static_cast<std::size_t>(i)], {x + 14.0f, rowY, 16.0f, 16.0f}, {139, 148, 158, 255});
        drawTextClipped(layerNames[static_cast<std::size_t>(i)], {x + 38.0f, rowY - 1.0f, width - 98.0f, 18.0f}, 14, {230, 237, 243, 255});
        const std::array<UiLayer, 5> layerIds{UiLayer::Flow, UiLayer::Resources, UiLayer::Persistence, UiLayer::Reliability, UiLayer::Geography};
        const bool enabled = context.state->enabledLayers[static_cast<std::size_t>(layerIds[static_cast<std::size_t>(i)])];
        drawToggle({x + width - 48.0f, rowY - 1.0f, 34.0f, 18.0f}, enabled);
    }

    y += layers.height + UiTheme::gap;
    Rectangle legend{x, y, width, std::max(130.0f, layout.leftSidebar.y + layout.leftSidebar.height - y)};
    drawPanelFrame(legend, "Legend");
    BeginScissorMode(static_cast<int>(legend.x), static_cast<int>(legend.y), static_cast<int>(legend.width), static_cast<int>(legend.height));
    const std::array<const char*, 5> names{"Client Region", "Service", "Database", "Cache", "Network Link"};
    const std::array<const char*, 5> icons{"node.client", "node.service", "node.database", "node.cache", "legend.link"};
    for (int i = 0; i < 5; ++i) {
        const float rowY = y + 42.0f + static_cast<float>(i) * 24.0f;
        IconRegistry::instance().drawIcon(icons[static_cast<std::size_t>(i)], {x + 14.0f, rowY, 15.0f, 15.0f}, {89, 196, 255, 255});
        drawTextClipped(names[static_cast<std::size_t>(i)], {x + 38.0f, rowY - 1.0f, width - 52.0f, 18.0f}, 14, {139, 148, 158, 255});
    }
    EndScissorMode();
}
