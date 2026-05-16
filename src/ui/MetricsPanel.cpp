#include "ui/MetricsPanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"
#include "ui/UiPrimitives.hpp"

#include "raylib.h"

#include <array>
#include <cstdio>

namespace {
void metricRow(const char* icon, const char* label, const char* value, float x, float y, Color valueColor)
{
    IconRegistry::instance().drawIcon(icon, {x, y + 1.0f, 16.0f, 16.0f}, valueColor);
    drawTextClipped(label, {x + 24.0f, y, 112.0f, 18.0f}, 14, {139, 148, 158, 255});
    drawTextClipped(value, {x + 150.0f, y, 82.0f, 18.0f}, 14, valueColor);
}
}

void MetricsPanel::update(UiContext&, const Simulation&)
{
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

    Rectangle overview{x, y, width, 196.0f};
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
