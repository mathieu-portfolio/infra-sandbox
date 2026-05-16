#include "ui/MetricsPanel.hpp"

#include "raylib.h"

#include <cstdio>

namespace {
void drawTextLine(const char* text, int x, int y, int size, Color color)
{
    DrawText(text, x, y, size, color);
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

    const auto& metrics = simulation.metrics();
    const int x = 18;
    const int y = 18;
    DrawRectangleRounded({10.0f, 10.0f, 360.0f, 318.0f}, 0.04f, 8, {22, 27, 34, 235});

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "FPS %d  |  %s  |  %.0fx", GetFPS(), context.paused ? "PAUSED" : "RUNNING", simulation.simulationSpeed());
    drawTextLine(buffer, x, y, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Input rate: %.1f req/s", metrics.inputRatePerSecond);
    drawTextLine(buffer, x, y + 30, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "API queue: %d", metrics.apiQueueDepth);
    drawTextLine(buffer, x, y + 56, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "DB queue: %d", metrics.databaseQueueDepth);
    drawTextLine(buffer, x, y + 82, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Processed: %.1f req/s", metrics.processedPerSecond);
    drawTextLine(buffer, x, y + 108, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Avg latency: %.2fs", metrics.averageLatencySeconds);
    drawTextLine(buffer, x, y + 134, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Timeouts: %.1f req/s", metrics.timeoutRatePerSecond);
    drawTextLine(buffer, x, y + 160, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Retries: %.1f req/s", metrics.retryRatePerSecond);
    drawTextLine(buffer, x, y + 186, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "API utilization: %.0f%%", metrics.apiUtilization * 100.0);
    drawTextLine(buffer, x, y + 212, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "DB utilization: %.0f%%", metrics.databaseUtilization * 100.0);
    drawTextLine(buffer, x, y + 238, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Cache: %s  hit %.0f%%", simulation.cacheEnabled() ? "on" : "off", metrics.cacheHitRate * 100.0);
    drawTextLine(buffer, x, y + 264, 18, {230, 237, 243, 255});
    std::snprintf(buffer, sizeof(buffer), "Bursts: %s", simulation.burstModeEnabled() ? "on" : "off");
    drawTextLine(buffer, x, y + 290, 18, {230, 237, 243, 255});
}
