#include "ui/actions/EventOverlay.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/actions/ActionText.hpp"
#include "ui/core/UiCore.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace {
struct EventPanelRow {
    std::string title;
    std::string detail;
    EventCategory category = EventCategory::TrafficEvent;
    std::string meta;
};

Color categoryColor(EventCategory category)
{
    switch (category) {
    case EventCategory::TrafficEvent:
    case EventCategory::DemandEvent:
    case EventCategory::GeographicEvent:
        return {89, 196, 255, 255};
    case EventCategory::InfrastructureEvent:
    case EventCategory::FailureEvent:
        return {245, 184, 76, 255};
    case EventCategory::ReliabilityEvent:
        return {235, 86, 100, 255};
    case EventCategory::RecoveryEvent:
        return {86, 210, 151, 255};
    case EventCategory::EducationalEvent:
        return {189, 135, 255, 255};
    }
    return {139, 148, 158, 255};
}

void drawEventRow(Rectangle row, const std::string& title, const std::string& detail, EventCategory category, const std::string& meta)
{
    DrawRectangleRounded(row, 0.055f, 8, {15, 22, 33, 236});
    DrawRectangleRoundedLines(row, 0.055f, 8, {70, 86, 104, 120});
    const Color accent = categoryColor(category);
    DrawCircle(static_cast<int>(row.x + 18.0f), static_cast<int>(row.y + row.height * 0.5f), 5.0f, accent);
    const float titleY = row.height >= 42.0f ? row.y + 10.0f : row.y + std::max(4.0f, row.height * 0.5f - 7.0f);
    drawTextClipped(title, {row.x + 34.0f, titleY, row.width - 188.0f, 18.0f}, 14, {241, 245, 249, 255});
    drawTextClipped(meta, {row.x + row.width - 140.0f, titleY, 124.0f, 16.0f}, 12, accent);
    if (row.height >= 48.0f) {
        actions_ui::drawWrappedTextClipped(detail, {row.x + 34.0f, row.y + 34.0f, row.width - 50.0f, row.height - 42.0f}, 12, {166, 176, 192, 255}, 3.0f);
    }
}
} // namespace

Rectangle EventOverlay::overlayBounds(int screenWidth, int screenHeight)
{
    const float width = std::min(760.0f, static_cast<float>(screenWidth) - 120.0f);
    const float height = std::min(430.0f, static_cast<float>(screenHeight) - 170.0f);
    return {static_cast<float>(screenWidth) * 0.5f - width * 0.5f, static_cast<float>(screenHeight) * 0.5f - height * 0.5f, width, height};
}

Rectangle EventOverlay::acknowledgeButtonBounds(Rectangle overlay)
{
    return {overlay.x + overlay.width - 190.0f, overlay.y + overlay.height - 52.0f, 168.0f, 34.0f};
}

void EventOverlay::draw(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr || context.state->gameplayPhase != GameplayPhase::Planning || !context.state->eventPanelVisible) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    DrawRectangleRec(layout.worldView, {0, 0, 0, 128});

    const Rectangle overlay = overlayBounds(context.screenWidth, context.screenHeight);
    DrawRectangleRounded(overlay, 0.025f, 8, {9, 16, 27, 248});
    DrawRectangleRoundedLines(overlay, 0.025f, 8, {70, 86, 104, 150});
    IconRegistry::instance().drawIcon("timeline.events", {overlay.x + 22.0f, overlay.y + 19.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText("EVENT REVIEW", static_cast<int>(overlay.x + 54.0f), static_cast<int>(overlay.y + 22.0f), 14, {139, 148, 158, 255});
    drawTextClipped("Review active and recent events before choosing the World Action for this planning phase.", {overlay.x + 22.0f, overlay.y + 50.0f, overlay.width - 44.0f, 20.0f}, 13, {205, 213, 224, 255});

    const auto& manager = scenarioManager.eventManager();
    std::vector<EventPanelRow> rows;
    for (const auto& active : manager.activeEvents()) {
        char meta[96];
        std::snprintf(meta, sizeof(meta), "%s | %.0fs left", eventCategoryName(active.definition.category), active.remainingSeconds);
        const std::string detail = active.definition.description.empty()
            ? eventLocationLabel(active.definition.location)
            : active.definition.description;
        rows.push_back({active.definition.name.empty() ? active.definition.displayName : active.definition.name, detail, active.definition.category, meta});
    }

    for (auto it = manager.recentEvents().rbegin(); it != manager.recentEvents().rend(); ++it) {
        const std::string detail = it->locationLabel.empty() ? "Global event pressure is part of the next planning context." : "Location: " + it->locationLabel;
        rows.push_back({it->name, detail, it->category, eventCategoryName(it->category)});
    }

    float y = overlay.y + 88.0f;
    if (rows.empty()) {
        DrawRectangleRounded({overlay.x + 22.0f, y, overlay.width - 44.0f, 86.0f}, 0.045f, 8, {15, 22, 33, 236});
        DrawRectangleRoundedLines({overlay.x + 22.0f, y, overlay.width - 44.0f, 86.0f}, 0.045f, 8, {70, 86, 104, 120});
        drawTextClipped("No active or recent events", {overlay.x + 42.0f, y + 20.0f, overlay.width - 84.0f, 18.0f}, 14, {230, 237, 243, 255});
        drawTextClipped("The World Action draft will use the current pressure profile.", {overlay.x + 42.0f, y + 48.0f, overlay.width - 84.0f, 16.0f}, 12, {139, 148, 158, 255});
    } else {
        const float listBottom = overlay.y + overlay.height - 70.0f;
        const float gap = rows.size() > 6 ? 4.0f : 10.0f;
        const float availableHeight = std::max(22.0f, listBottom - y - gap * static_cast<float>(rows.size() - 1));
        const float rowHeight = std::min(66.0f, availableHeight / static_cast<float>(rows.size()));
        for (const auto& row : rows) {
            drawEventRow({overlay.x + 22.0f, y, overlay.width - 44.0f, rowHeight}, row.title, row.detail, row.category, row.meta);
            y += rowHeight + gap;
        }
    }

    const Rectangle button = acknowledgeButtonBounds(overlay);
    DrawRectangleRounded(button, 0.18f, 8, {50, 58, 70, 235});
    DrawRectangleRoundedLines(button, 0.18f, 8, {139, 148, 158, 150});
    drawTextClipped("Continue to World Actions", {button.x + 14.0f, button.y + 10.0f, button.width - 28.0f, 16.0f}, 12, {230, 237, 243, 255});
}
