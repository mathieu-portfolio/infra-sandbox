#include "ui/actions/EventOverlay.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/actions/ActionText.hpp"
#include "ui/core/UiCore.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
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

Rectangle nodeBounds(const ui::UiNode& root, const std::string& id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
}

float wrappedTextHeight(const std::string& text, float width, int fontSize, int maxLines)
{
    if (text.empty()) {
        return static_cast<float>(fontSize);
    }
    const float avgCharWidth = static_cast<float>(fontSize) * 0.56f;
    const int charsPerLine = std::max(1, static_cast<int>(width / std::max(1.0f, avgCharWidth)));
    int lines = 1;
    int current = 0;
    for (char c : text) {
        if (c == '\n') {
            ++lines;
            current = 0;
            continue;
        }
        ++current;
        if (current >= charsPerLine) {
            ++lines;
            current = 0;
        }
    }
    return static_cast<float>(std::clamp(lines, 1, std::max(1, maxLines)) * fontSize);
}

float eventRowHeight(const EventLogEntry& event, float rowWidth, bool planning)
{
    const std::string detail = event.description.empty()
        ? (event.locationLabel.empty() ? "World state changed during simulation." : "Location: " + event.locationLabel)
        : event.description;
    const float detailWidth = std::max(1.0f, rowWidth - 50.0f);
    const int maxLines = planning ? 4 : 3;
    return std::clamp(48.0f + wrappedTextHeight(detail, detailWidth, 12, maxLines), 66.0f, planning ? 112.0f : 92.0f);
}

void drawEventRow(Rectangle row, const EventLogEntry& event, bool planning)
{
    DrawRectangleRounded(row, 0.055f, 8, {15, 22, 33, 236});
    DrawRectangleRoundedLines(row, 0.055f, 8, {70, 86, 104, 120});
    const Color accent = categoryColor(event.category);
    DrawCircle(static_cast<int>(row.x + 18.0f), static_cast<int>(row.y + 20.0f), 5.0f, accent);
    drawTextClipped(event.name, {row.x + 34.0f, row.y + 10.0f, row.width - 188.0f, 18.0f}, 14, {241, 245, 249, 255});
    drawTextClipped(eventCategoryName(event.category), {row.x + row.width - 140.0f, row.y + 10.0f, 124.0f, 16.0f}, 12, accent);
    const std::string detail = event.description.empty()
        ? (event.locationLabel.empty() ? "World state changed during simulation." : "Location: " + event.locationLabel)
        : event.description;
    actions_ui::drawWrappedTextClipped(detail, {row.x + 34.0f, row.y + 34.0f, row.width - 50.0f, row.height - 42.0f}, 12, {166, 176, 192, 255}, planning ? 4.0f : 3.0f);
}

struct EventOverlayLayout {
    std::unique_ptr<ui::StackNode> root;
    Rectangle overlay{};
    Rectangle title{};
    Rectangle intro{};
    Rectangle empty{};
    Rectangle button{};
    std::vector<Rectangle> rows;
};

EventOverlayLayout buildEventOverlayLayout(int screenWidth, int screenHeight, EventPopupMode mode, const std::vector<EventLogEntry>& events)
{
    const bool planning = mode == EventPopupMode::PlanningStart;
    const float screenW = static_cast<float>(screenWidth);
    const float screenH = static_cast<float>(screenHeight);
    const float width = std::clamp(screenW - 120.0f, 360.0f, 760.0f);
    const float maxHeight = std::max(220.0f, screenH - 170.0f);
    const float rowWidth = width - 44.0f;

    auto root = ui::verticalStack("eventOverlay");
    ui::LayoutStyle rootStyle = ui::fixedWidth(width);
    rootStyle.paddingLeft = 22.0f;
    rootStyle.paddingTop = 18.0f;
    rootStyle.paddingRight = 22.0f;
    rootStyle.paddingBottom = 18.0f;
    rootStyle.gap = 14.0f;
    rootStyle.maxHeight = maxHeight;
    root->style(rootStyle);

    auto title = std::make_unique<ui::PanelNode>("title");
    title->style(ui::fixedHeight(22.0f));
    root->add(std::move(title));

    auto intro = std::make_unique<ui::TextBlockNode>(
        planning ? "A new event shaped the planning context. Adapt your actions before advancing time."
                 : "These events occurred while simulation time advanced. You can react during the next planning phase.",
        13,
        "intro");
    intro->color = {205, 213, 224, 255};
    intro->maxLines = 3;
    root->add(std::move(intro));

    auto rows = ui::verticalStack("rows");
    ui::LayoutStyle rowsStyle = ui::contentSize();
    rowsStyle.gap = 10.0f;
    rows->style(rowsStyle);

    if (events.empty()) {
        auto empty = std::make_unique<ui::PanelNode>("empty");
        empty->style(ui::fixedHeight(86.0f));
        rows->add(std::move(empty));
    } else {
        const std::size_t limit = planning ? 1u : events.size();
        for (std::size_t i = 0; i < limit; ++i) {
            auto row = std::make_unique<ui::PanelNode>("row" + std::to_string(i));
            row->style(ui::fixedHeight(eventRowHeight(events[i], rowWidth, planning)));
            rows->add(std::move(row));
        }
    }
    root->add(std::move(rows));

    auto buttonRow = ui::horizontalStack("buttonRow");
    ui::LayoutStyle buttonRowStyle = ui::contentSize();
    buttonRowStyle.crossAlign = ui::Align::Center;
    buttonRow->style(buttonRowStyle);
    auto spacer = std::make_unique<ui::PanelNode>("buttonSpacer");
    ui::LayoutStyle spacerStyle;
    spacerStyle.widthMode = ui::SizeMode::Flex;
    spacerStyle.flexGrow = 1.0f;
    spacerStyle.heightMode = ui::SizeMode::Content;
    spacer->style(spacerStyle);
    buttonRow->add(std::move(spacer));
    auto button = std::make_unique<ui::ButtonNode>(planning ? "Continue to World Actions" : "Continue to Analysis", 12, "acknowledgeButton");
    buttonRow->add(std::move(button));
    root->add(std::move(buttonRow));

    const ui::Size measured = root->measure({width, maxHeight});
    const float height = std::min(maxHeight, measured.height);
    const Rectangle overlay{screenW * 0.5f - width * 0.5f, screenH * 0.5f - height * 0.5f, width, height};
    root->layout(overlay);

    EventOverlayLayout layout;
    layout.overlay = overlay;
    layout.title = nodeBounds(*root, "title");
    layout.intro = nodeBounds(*root, "intro");
    layout.empty = nodeBounds(*root, "empty");
    layout.button = nodeBounds(*root, "acknowledgeButton");
    const std::size_t rowCount = events.empty() ? 0u : (planning ? 1u : events.size());
    for (std::size_t i = 0; i < rowCount; ++i) {
        layout.rows.push_back(nodeBounds(*root, "row" + std::to_string(i)));
    }
    layout.root = std::move(root);
    return layout;
}
} // namespace

Rectangle EventOverlay::overlayBounds(int screenWidth, int screenHeight)
{
    return buildEventOverlayLayout(screenWidth, screenHeight, EventPopupMode::PlanningStart, {}).overlay;
}

Rectangle EventOverlay::acknowledgeButtonBounds(Rectangle overlay)
{
    const float width = static_cast<float>(MeasureText("Continue to World Actions", 12)) + 28.0f;
    const float height = 30.0f;
    return {overlay.x + overlay.width - width - 22.0f, overlay.y + overlay.height - height - 18.0f, width, height};
}

void EventOverlay::draw(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    (void)scenarioManager;
    if (context.state == nullptr || context.state->eventPopupMode == EventPopupMode::None) {
        return;
    }

    const UiLayout uiLayout = computeUiLayout(context.screenWidth, context.screenHeight);
    DrawRectangleRec(uiLayout.worldView, {0, 0, 0, 128});

    const bool planning = context.state->eventPopupMode == EventPopupMode::PlanningStart;
    const EventOverlayLayout layout = buildEventOverlayLayout(
        context.screenWidth,
        context.screenHeight,
        context.state->eventPopupMode,
        context.state->eventPopupEvents);

    DrawRectangleRounded(layout.overlay, 0.025f, 8, {9, 16, 27, 248});
    DrawRectangleRoundedLines(layout.overlay, 0.025f, 8, {70, 86, 104, 150});

    IconRegistry::instance().drawIcon("timeline.events", {layout.title.x, layout.title.y + 1.0f, 22.0f, 22.0f}, {139, 148, 158, 255});
    DrawText(planning ? "EVENT BRIEFING" : "SIMULATION EVENT RECAP", static_cast<int>(layout.title.x + 32.0f), static_cast<int>(layout.title.y + 4.0f), 14, {139, 148, 158, 255});
    drawTextClipped(
        planning ? "A new event shaped the planning context. Adapt your actions before advancing time."
                 : "These events occurred while simulation time advanced. You can react during the next planning phase.",
        layout.intro,
        13,
        {205, 213, 224, 255});

    if (context.state->eventPopupEvents.empty()) {
        DrawRectangleRounded(layout.empty, 0.045f, 8, {15, 22, 33, 236});
        DrawRectangleRoundedLines(layout.empty, 0.045f, 8, {70, 86, 104, 120});
        drawTextClipped("No event", {layout.empty.x + 20.0f, layout.empty.y + 20.0f, layout.empty.width - 40.0f, 18.0f}, 14, {230, 237, 243, 255});
        drawTextClipped("No eligible event was applied for this phase.", {layout.empty.x + 20.0f, layout.empty.y + 48.0f, layout.empty.width - 40.0f, 16.0f}, 12, {139, 148, 158, 255});
    } else {
        const std::size_t count = std::min(layout.rows.size(), context.state->eventPopupEvents.size());
        for (std::size_t i = 0; i < count; ++i) {
            drawEventRow(layout.rows[i], context.state->eventPopupEvents[i], planning);
        }
    }

    DrawRectangleRounded(layout.button, 0.18f, 8, {50, 58, 70, 235});
    DrawRectangleRoundedLines(layout.button, 0.18f, 8, {139, 148, 158, 150});
    drawTextClipped(planning ? "Continue to World Actions" : "Continue to Analysis", {layout.button.x + 14.0f, layout.button.y + 8.0f, layout.button.width - 28.0f, 16.0f}, 12, {230, 237, 243, 255});
}
