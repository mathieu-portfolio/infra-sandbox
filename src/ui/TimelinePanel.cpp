#include "ui/TimelinePanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>
#include <deque>

namespace {
float normalizedValue(const MetricsSnapshot& metrics, int series)
{
    switch (series) {
    case 0:
        return static_cast<float>(std::clamp(metrics.inputRatePerSecond / 25.0, 0.0, 1.0));
    case 1:
        return static_cast<float>(std::clamp(metrics.averageLatencySeconds / 4.0, 0.0, 1.0));
    case 2:
        return static_cast<float>(std::clamp(static_cast<double>(metrics.apiQueueDepth + metrics.databaseQueueDepth) / 80.0, 0.0, 1.0));
    case 3:
        return static_cast<float>(std::clamp((metrics.apiUtilization + metrics.databaseUtilization) * 0.5, 0.0, 1.0));
    case 4:
        return static_cast<float>(std::clamp(metrics.cacheHitRate, 0.0, 1.0));
    }
    return 0.0f;
}

void drawChart(Rectangle bounds, const char* title, Color color, const std::deque<MetricsSnapshot>& history, int series)
{
    DrawRectangleRounded(bounds, 0.035f, 6, {22, 27, 34, 220});
    DrawRectangleRoundedLines(bounds, 0.035f, 6, {70, 86, 104, 90});
    DrawText(title, static_cast<int>(bounds.x + 10.0f), static_cast<int>(bounds.y + 8.0f), 13, {230, 237, 243, 255});
    const float chartX = bounds.x + 12.0f;
    const float chartY = bounds.y + 40.0f;
    const float chartW = bounds.width - 24.0f;
    const float chartH = bounds.height - 54.0f;
    DrawRectangleLinesEx({chartX, chartY, chartW, chartH}, 1.0f, {70, 86, 104, 65});
    if (history.empty()) {
        return;
    }

    Vector2 prev{chartX, chartY + chartH * (1.0f - normalizedValue(history.front(), series))};
    const int count = static_cast<int>(history.size());
    for (int i = 1; i < count; ++i) {
        const float t = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.0f;
        const float value = normalizedValue(history[static_cast<std::size_t>(i)], series);
        Vector2 cur{chartX + chartW * t, chartY + chartH * (1.0f - value)};
        DrawLineEx(prev, cur, 2.0f, color);
        prev = cur;
    }
}
}

void TimelinePanel::update(UiContext&, const Simulation&)
{
}

void TimelinePanel::draw(const UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const Rectangle panel = layout.bottomPanel;
    DrawRectangleRounded(panel, 0.025f, 8, {13, 17, 23, 232});
    DrawRectangleRoundedLines(panel, 0.025f, 8, {70, 86, 104, 95});
    DrawText("Metrics", static_cast<int>(panel.x + 14.0f), static_cast<int>(panel.y + 10.0f), 16, {89, 196, 255, 255});
    DrawText("Activity Timeline", static_cast<int>(panel.x + 14.0f), static_cast<int>(panel.y + 142.0f), 16, {89, 196, 255, 255});

    (void)simulation;
    const float chartY = panel.y + 38.0f;
    const float chartW = (panel.width - 64.0f) / 5.0f;
    drawChart({panel.x + 12.0f, chartY, chartW, 92.0f}, "Traffic", {89, 196, 255, 255}, context.state->metricsHistory, 0);
    drawChart({panel.x + 24.0f + chartW, chartY, chartW, 92.0f}, "Latency", {245, 184, 76, 255}, context.state->metricsHistory, 1);
    drawChart({panel.x + 36.0f + chartW * 2.0f, chartY, chartW, 92.0f}, "Queues", {235, 105, 76, 255}, context.state->metricsHistory, 2);
    drawChart({panel.x + 48.0f + chartW * 3.0f, chartY, chartW, 92.0f}, "Utilization", {86, 210, 151, 255}, context.state->metricsHistory, 3);
    drawChart({panel.x + 60.0f + chartW * 4.0f, chartY, chartW, 92.0f}, "Cache", {151, 111, 255, 255}, context.state->metricsHistory, 4);

    int row = 0;
    const int timelineY = static_cast<int>(panel.y + 172.0f);
    for (auto it = context.state->actionHistory.rbegin(); it != context.state->actionHistory.rend() && row < 2; ++it) {
        char buffer[180];
        std::snprintf(buffer, sizeof(buffer), "%.0fs  %s  %s", it->timeSeconds, it->actionName.c_str(), it->message.c_str());
        IconRegistry::instance().drawIcon("action.generic", {panel.x + 14.0f, static_cast<float>(timelineY + row * 22), 15.0f, 15.0f}, {89, 196, 255, 255});
        DrawText(buffer, static_cast<int>(panel.x + 38.0f), timelineY + row * 22, 13, {230, 237, 243, 255});
        ++row;
    }
    for (auto it = scenarioManager.eventManager().recentEvents().rbegin(); it != scenarioManager.eventManager().recentEvents().rend() && row < 4; ++it) {
        char buffer[160];
        std::snprintf(buffer, sizeof(buffer), "%.0fs  %s", it->timeSeconds, it->name.c_str());
        IconRegistry::instance().drawIcon("alert.warning", {panel.x + 14.0f, static_cast<float>(timelineY + row * 22), 15.0f, 15.0f}, {245, 184, 76, 255});
        DrawText(buffer, static_cast<int>(panel.x + 38.0f), timelineY + row * 22, 13, {139, 148, 158, 255});
        ++row;
    }
}
