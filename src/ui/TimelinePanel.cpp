#include "ui/TimelinePanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"
#include "ui/UiPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace {
float defaultSeriesMaxValue(int series)
{
    switch (series) {
    case 0:
        return 300.0f;
    case 1:
        return 600.0f;
    case 2:
        return 100.0f;
    case 3:
        return 100.0f;
    case 4:
        return 100.0f;
    }
    return 1.0f;
}

float rawValue(const MetricsSnapshot& metrics, int series)
{
    switch (series) {
    case 0:
        return static_cast<float>(metrics.inputRatePerSecond);
    case 1:
        return static_cast<float>(metrics.averageLatencySeconds * 1000.0);
    case 2:
        return static_cast<float>(metrics.apiQueueDepth + metrics.databaseQueueDepth);
    case 3:
        return static_cast<float>((metrics.apiUtilization + metrics.databaseUtilization) * 50.0);
    case 4:
        return static_cast<float>(metrics.cacheHitRate * 100.0);
    }
    return 0.0f;
}

float niceCeil(float value)
{
    if (value <= 0.0f) {
        return 1.0f;
    }

    float scale = 1.0f;
    while (value / scale >= 10.0f) {
        scale *= 10.0f;
    }
    while (value / scale < 1.0f) {
        scale *= 0.1f;
    }

    const float normalized = value / scale;
    float rounded = 10.0f;
    if (normalized <= 1.0f) {
        rounded = 1.0f;
    } else if (normalized <= 2.0f) {
        rounded = 2.0f;
    } else if (normalized <= 5.0f) {
        rounded = 5.0f;
    }
    return rounded * scale;
}

float visibleSeriesMax(const std::deque<MetricsSnapshot>& history, int series)
{
    float maxValue = 0.0f;
    for (const auto& sample : history) {
        maxValue = std::max(maxValue, rawValue(sample, series));
    }
    return niceCeil(std::max(maxValue, defaultSeriesMaxValue(series) * 0.1f));
}

float normalizedValue(const MetricsSnapshot& metrics, int series, float maxValue)
{
    return maxValue > 0.0f ? std::clamp(rawValue(metrics, series) / maxValue, 0.0f, 1.0f) : 0.0f;
}

void drawChartScale(Rectangle chartBounds, float maxValue)
{
    const Color labelColor{139, 148, 158, 210};
    const Color gridColor{70, 86, 104, 65};
    for (int i = 0; i <= 2; ++i) {
        const float ratio = static_cast<float>(i) / 2.0f;
        const float y = chartBounds.y + chartBounds.height * (1.0f - ratio);
        DrawLineEx({chartBounds.x, y}, {chartBounds.x + chartBounds.width, y}, 1.0f, gridColor);
        char label[16];
        std::snprintf(label, sizeof(label), "%.0f", maxValue * ratio);
        DrawText(label, static_cast<int>(chartBounds.x - 31.0f), static_cast<int>(y - 6.0f), 10, labelColor);
    }

    DrawText("-5m", static_cast<int>(chartBounds.x - 2.0f), static_cast<int>(chartBounds.y + chartBounds.height + 6.0f), 10, labelColor);
    DrawText("-1m", static_cast<int>(chartBounds.x + chartBounds.width * 0.72f), static_cast<int>(chartBounds.y + chartBounds.height + 6.0f), 10, labelColor);
    DrawText("Now", static_cast<int>(chartBounds.x + chartBounds.width - 20.0f), static_cast<int>(chartBounds.y + chartBounds.height + 6.0f), 10, labelColor);
}

void drawChart(Rectangle bounds, const char* title, Color color, const std::deque<MetricsSnapshot>& history, int series)
{
    DrawRectangleRounded(bounds, 0.035f, 6, {22, 27, 34, 220});
    DrawRectangleRoundedLines(bounds, 0.035f, 6, {70, 86, 104, 90});
    DrawText(title, static_cast<int>(bounds.x + 10.0f), static_cast<int>(bounds.y + 8.0f), 13, {230, 237, 243, 255});
    const float chartX = bounds.x + 40.0f;
    const float chartY = bounds.y + 40.0f;
    const float chartW = bounds.width - 52.0f;
    const float chartH = bounds.height - 66.0f;
    const Rectangle chartBounds{chartX, chartY, chartW, chartH};
    DrawRectangleLinesEx(chartBounds, 1.0f, {70, 86, 104, 65});
    const float maxValue = visibleSeriesMax(history, series);
    drawChartScale(chartBounds, maxValue);
    if (history.empty()) {
        return;
    }

    Vector2 prev{chartX, chartY + chartH * (1.0f - normalizedValue(history.front(), series, maxValue))};
    const int count = static_cast<int>(history.size());
    for (int i = 1; i < count; ++i) {
        const float t = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.0f;
        const float value = normalizedValue(history[static_cast<std::size_t>(i)], series, maxValue);
        Vector2 cur{chartX + chartW * t, chartY + chartH * (1.0f - value)};
        DrawLineEx(prev, cur, 2.0f, color);
        prev = cur;
    }
}

struct ActivityRow {
    double timeSeconds = 0.0;
    TimelineCategory category = TimelineCategory::System;
    std::string title;
    std::string detail;
    bool active = false;
};

Rectangle categoryDroplistBounds(Rectangle panel)
{
    const float chartHeight = std::clamp(panel.height * 0.37f, 92.0f, 128.0f);
    return {panel.x + panel.width - 246.0f, panel.y + 38.0f + chartHeight + 10.0f, 112.0f, 28.0f};
}

Rectangle filterDroplistBounds(Rectangle panel)
{
    const float chartHeight = std::clamp(panel.height * 0.37f, 92.0f, 128.0f);
    return {panel.x + panel.width - 126.0f, panel.y + 38.0f + chartHeight + 10.0f, 112.0f, 28.0f};
}

Color categoryColor(TimelineCategory category)
{
    switch (category) {
    case TimelineCategory::Traffic:
        return {89, 196, 255, 255};
    case TimelineCategory::Change:
        return {86, 210, 151, 255};
    case TimelineCategory::Database:
        return {245, 184, 76, 255};
    case TimelineCategory::Objectives:
        return {151, 111, 255, 255};
    case TimelineCategory::Reliability:
        return {235, 86, 100, 255};
    case TimelineCategory::System:
    case TimelineCategory::All:
        return {139, 148, 158, 255};
    }
    return {139, 148, 158, 255};
}

TimelineCategory categoryForEvent(EventCategory category)
{
    switch (category) {
    case EventCategory::TrafficEvent:
    case EventCategory::DemandEvent:
    case EventCategory::GeographicEvent:
        return TimelineCategory::Traffic;
    case EventCategory::InfrastructureEvent:
    case EventCategory::RecoveryEvent:
        return TimelineCategory::Change;
    case EventCategory::FailureEvent:
        return TimelineCategory::Database;
    case EventCategory::ReliabilityEvent:
        return TimelineCategory::Reliability;
    case EventCategory::EducationalEvent:
        return TimelineCategory::System;
    }
    return TimelineCategory::System;
}

bool rowMatches(const ActivityRow& row, const UiState& state)
{
    if (state.timelineCategory != TimelineCategory::All && row.category != state.timelineCategory) {
        return false;
    }
    if (state.timelineFilter == TimelineFilter::ActiveOnly && !row.active) {
        return false;
    }
    return true;
}

std::vector<ActivityRow> buildActivityRows(const UiState& state, const Simulation& simulation, const ScenarioManager& scenarioManager)
{
    std::vector<ActivityRow> rows;
    rows.push_back({0.0, TimelineCategory::System, "Scenario started", scenarioManager.staticDefinition().name, true});

    const auto& run = scenarioManager.run();
    for (const auto& id : run.activeObjectiveIds) {
        const auto it = std::find_if(scenarioManager.definition().objectives.begin(), scenarioManager.definition().objectives.end(), [&id](const ScenarioObjective& objective) { return objective.id == id; });
        if (it != scenarioManager.definition().objectives.end()) {
            rows.push_back({run.elapsedSeconds, TimelineCategory::Objectives, "Objective active", it->summary, true});
        }
    }
    for (const auto& id : run.completedObjectiveIds) {
        const auto it = std::find_if(scenarioManager.definition().objectives.begin(), scenarioManager.definition().objectives.end(), [&id](const ScenarioObjective& objective) { return objective.id == id; });
        if (it != scenarioManager.definition().objectives.end()) {
            rows.push_back({run.elapsedSeconds, TimelineCategory::Objectives, "Objective completed", it->summary, false});
        }
    }
    for (const auto& failure : scenarioManager.definition().failureConditions) {
        rows.push_back({run.elapsedSeconds, TimelineCategory::Objectives, "Failure limit monitored", failure.summary, true});
    }

    for (const auto& entry : state.actionHistory) {
        rows.push_back({entry.timeSeconds, TimelineCategory::Change, entry.actionName, entry.message.empty() ? entry.target : entry.message, entry.observationPending});
    }
    for (const auto& planned : state.plannedInterventions) {
        rows.push_back({simulation.timeSeconds(), TimelineCategory::Change, "Planned intervention", planned.actionName + " -> " + planned.target, true});
    }
    for (const auto& summary : state.resolutionSummaries) {
        rows.push_back({simulation.timeSeconds(), TimelineCategory::System, "Resolution", summary, true});
    }

    for (const auto& pressureEvent : simulation.pressure().recentEvents) {
        rows.push_back({pressureEvent.timeSeconds, TimelineCategory::System, "Pressure observed", pressureEvent.summary, true});
    }
    for (const auto& pattern : simulation.pressure().suspiciousPatterns) {
        rows.push_back({simulation.timeSeconds(), TimelineCategory::System, "Suspicious pattern", pattern, true});
    }

    for (const auto& event : scenarioManager.eventManager().recentEvents()) {
        rows.push_back({event.timeSeconds, categoryForEvent(event.category), event.name, eventCategoryName(event.category), true});
    }

    std::sort(rows.begin(), rows.end(), [&state](const ActivityRow& lhs, const ActivityRow& rhs) {
        if (state.timelineFilter == TimelineFilter::OldestFirst) {
            return lhs.timeSeconds < rhs.timeSeconds;
        }
        return lhs.timeSeconds > rhs.timeSeconds;
    });
    return rows;
}

void drawDroplist(Rectangle bounds, const char* label, bool open)
{
    DrawRectangleRounded(bounds, 0.14f, 6, {22, 27, 34, 235});
    DrawRectangleRoundedLines(bounds, 0.14f, 6, open ? Color{89, 196, 255, 200} : Color{70, 86, 104, 120});
    drawTextClipped(label, {bounds.x + 9.0f, bounds.y + 7.0f, bounds.width - 28.0f, 15.0f}, 12, {230, 237, 243, 255});
    const float x = bounds.x + bounds.width - 15.0f;
    const float y = bounds.y + bounds.height * 0.5f;
    DrawTriangle({x - 4.0f, y - 2.0f}, {x + 4.0f, y - 2.0f}, {x, y + 4.0f}, {139, 148, 158, 255});
}

void drawCategoryBadge(Rectangle bounds, TimelineCategory category)
{
    const Color color = categoryColor(category);
    DrawRectangleRounded(bounds, 0.35f, 8, {color.r, color.g, color.b, 28});
    DrawRectangleRoundedLines(bounds, 0.35f, 8, {color.r, color.g, color.b, 95});
    drawTextClipped(timelineCategoryName(category), {bounds.x + 8.0f, bounds.y + 3.0f, bounds.width - 16.0f, 14.0f}, 11, color);
}

void drawTimelineMenu(Rectangle field, const std::vector<const char*>& labels)
{
    const Rectangle menu{field.x, field.y + field.height + 4.0f, field.width, static_cast<float>(labels.size()) * 24.0f + 8.0f};
    DrawRectangleRounded(menu, 0.06f, 6, {14, 20, 28, 246});
    DrawRectangleRoundedLines(menu, 0.06f, 6, {89, 196, 255, 130});
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        drawTextClipped(labels[static_cast<std::size_t>(i)], {menu.x + 8.0f, menu.y + 6.0f + static_cast<float>(i) * 24.0f, menu.width - 16.0f, 16.0f}, 12, {230, 237, 243, 255});
    }
}
}

void TimelinePanel::update(UiContext& context, const Simulation&, const ScenarioManager&)
{
    if (context.state == nullptr || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const Rectangle categoryField = categoryDroplistBounds(layout.bottomPanel);
    const Rectangle filterField = filterDroplistBounds(layout.bottomPanel);
    const Vector2 mouse = GetMousePosition();
    constexpr std::array<TimelineCategory, 7> categories{
        TimelineCategory::All,
        TimelineCategory::Objectives,
        TimelineCategory::Traffic,
        TimelineCategory::Change,
        TimelineCategory::Database,
        TimelineCategory::Reliability,
        TimelineCategory::System,
    };
    constexpr std::array<TimelineFilter, 3> filters{
        TimelineFilter::RecentFirst,
        TimelineFilter::OldestFirst,
        TimelineFilter::ActiveOnly,
    };

    if (CheckCollisionPointRec(mouse, categoryField)) {
        context.state->timelineCategoryDroplistOpen = !context.state->timelineCategoryDroplistOpen;
        context.state->timelineFilterDroplistOpen = false;
        return;
    }
    if (CheckCollisionPointRec(mouse, filterField)) {
        context.state->timelineFilterDroplistOpen = !context.state->timelineFilterDroplistOpen;
        context.state->timelineCategoryDroplistOpen = false;
        return;
    }

    if (context.state->timelineCategoryDroplistOpen) {
        const Rectangle menu{categoryField.x, categoryField.y + categoryField.height + 4.0f, categoryField.width, static_cast<float>(categories.size()) * 24.0f + 8.0f};
        for (int i = 0; i < static_cast<int>(categories.size()); ++i) {
            const Rectangle row{menu.x, menu.y + 4.0f + static_cast<float>(i) * 24.0f, menu.width, 24.0f};
            if (CheckCollisionPointRec(mouse, row)) {
                context.state->timelineCategory = categories[static_cast<std::size_t>(i)];
                context.state->timelineCategoryDroplistOpen = false;
                return;
            }
        }
        context.state->timelineCategoryDroplistOpen = CheckCollisionPointRec(mouse, menu);
    }

    if (context.state->timelineFilterDroplistOpen) {
        const Rectangle menu{filterField.x, filterField.y + filterField.height + 4.0f, filterField.width, static_cast<float>(filters.size()) * 24.0f + 8.0f};
        for (int i = 0; i < static_cast<int>(filters.size()); ++i) {
            const Rectangle row{menu.x, menu.y + 4.0f + static_cast<float>(i) * 24.0f, menu.width, 24.0f};
            if (CheckCollisionPointRec(mouse, row)) {
                context.state->timelineFilter = filters[static_cast<std::size_t>(i)];
                context.state->timelineFilterDroplistOpen = false;
                return;
            }
        }
        context.state->timelineFilterDroplistOpen = CheckCollisionPointRec(mouse, menu);
    }
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
    const float chartHeight = std::clamp(panel.height * 0.37f, 92.0f, 128.0f);
    const float timelineHeaderY = panel.y + 38.0f + chartHeight + 12.0f;
    DrawText("Activity Timeline", static_cast<int>(panel.x + 14.0f), static_cast<int>(timelineHeaderY), 16, {89, 196, 255, 255});
    const Rectangle categoryField = categoryDroplistBounds(panel);
    const Rectangle filterField = filterDroplistBounds(panel);
    drawDroplist(categoryField, timelineCategoryName(context.state->timelineCategory), context.state->timelineCategoryDroplistOpen);
    drawDroplist(filterField, timelineFilterName(context.state->timelineFilter), context.state->timelineFilterDroplistOpen);

    (void)simulation;
    const float chartY = panel.y + 38.0f;
    const float chartW = (panel.width - 64.0f) / 5.0f;
    drawChart({panel.x + 12.0f, chartY, chartW, chartHeight}, "Traffic (req/s)", {89, 196, 255, 255}, context.state->metricsHistory, 0);
    drawChart({panel.x + 24.0f + chartW, chartY, chartW, chartHeight}, "Latency (ms)", {245, 184, 76, 255}, context.state->metricsHistory, 1);
    drawChart({panel.x + 36.0f + chartW * 2.0f, chartY, chartW, chartHeight}, "Queues (req)", {235, 105, 76, 255}, context.state->metricsHistory, 2);
    drawChart({panel.x + 48.0f + chartW * 3.0f, chartY, chartW, chartHeight}, "Utilization (%)", {86, 210, 151, 255}, context.state->metricsHistory, 3);
    drawChart({panel.x + 60.0f + chartW * 4.0f, chartY, chartW, chartHeight}, "Cache (%)", {151, 111, 255, 255}, context.state->metricsHistory, 4);

    int row = 0;
    const int timelineY = static_cast<int>(timelineHeaderY + 30.0f);
    const int maxRows = std::max(4, static_cast<int>((panel.y + panel.height - static_cast<float>(timelineY) - 12.0f) / 22.0f));
    const auto rows = buildActivityRows(*context.state, simulation, scenarioManager);
    for (const auto& activity : rows) {
        if (row >= maxRows || !rowMatches(activity, *context.state)) {
            continue;
        }
        const float y = static_cast<float>(timelineY + row * 22);
        const Color color = categoryColor(activity.category);
        char timeBuffer[24];
        std::snprintf(timeBuffer, sizeof(timeBuffer), "%.0fs", activity.timeSeconds);
        IconRegistry::instance().drawIcon(activity.category == TimelineCategory::Objectives ? "metric.objective" : "action.generic", {panel.x + 14.0f, y, 15.0f, 15.0f}, color);
        DrawText(timeBuffer, static_cast<int>(panel.x + 38.0f), static_cast<int>(y), 12, {139, 148, 158, 255});
        drawTextClipped(activity.title, {panel.x + 88.0f, y, 148.0f, 16.0f}, 13, {230, 237, 243, 255});
        drawTextClipped(activity.detail, {panel.x + 250.0f, y, panel.width - 410.0f, 16.0f}, 12, {139, 148, 158, 255});
        drawCategoryBadge({panel.x + panel.width - 118.0f, y - 1.0f, 104.0f, 18.0f}, activity.category);
        ++row;
    }

    if (row == 0) {
        drawTextClipped("No events match the selected filters.", {panel.x + 14.0f, static_cast<float>(timelineY), panel.width - 28.0f, 18.0f}, 13, {139, 148, 158, 255});
    }

    if (context.state->timelineCategoryDroplistOpen) {
        drawTimelineMenu(categoryField, {"All Events", "Objectives", "Traffic", "Change", "Database", "Reliability", "System"});
    }
    if (context.state->timelineFilterDroplistOpen) {
        drawTimelineMenu(filterField, {"Recent First", "Oldest First", "Active Only"});
    }
}
