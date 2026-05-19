#include "ui/actions/ActionPanel.hpp"

#include "simulation/NodeDefinition.hpp"
#include "ui/actions/ActionPanelModel.hpp"
#include "ui/actions/cards/NodeActionCardView.hpp"
#include "ui/IconRegistry.hpp"
#include "ui/RightSidebarLayout.hpp"
#include "ui/core/UiCore.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"
#include "ui/actions/ActionFiltering.hpp"
#include "ui/actions/ActionText.hpp"
#include "ui/actions/PressurePresentation.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

namespace {

Rectangle nodeBounds(const ui::UiNode& root, const char* id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
}

struct ActionHeaderLayout {
    Rectangle title;
    Rectangle capacity;
    Rectangle worldStatus;
    Rectangle filters[4]{};
};

ActionHeaderLayout computeActionHeaderLayout(Rectangle bounds)
{
    auto root = ui::verticalStack("actionHeaderRoot");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = 8.0f;
    root->style(rootStyle);

    auto title = std::make_unique<ui::PanelNode>("title");
    title->style(ui::fixedHeight(18.0f));
    root->add(std::move(title));

    auto capacity = std::make_unique<ui::PanelNode>("capacity");
    capacity->style(ui::fixedHeight(140.0f));
    root->add(std::move(capacity));

    auto filters = ui::grid(4, "filters");
    ui::LayoutStyle filtersStyle;
    filtersStyle.heightMode = ui::SizeMode::Fixed;
    filtersStyle.fixedHeight = 26.0f;
    filtersStyle.widthMode = ui::SizeMode::Flex;
    filtersStyle.flexGrow = 1.0f;
    filters->style(filtersStyle);
    filters->columnGap = 10.0f;
    filters->fixedCellHeight = 26.0f;
    for (int i = 0; i < 4; ++i) {
        auto filter = std::make_unique<ui::PanelNode>("filter" + std::to_string(i));
        filter->style(ui::fixedHeight(26.0f));
        filters->add(std::move(filter));
    }
    root->add(std::move(filters));

    root->measure({bounds.width, bounds.height});
    root->layout(bounds);

    ActionHeaderLayout layout;
    layout.title = nodeBounds(*root, "title");
    layout.capacity = nodeBounds(*root, "capacity");
    layout.worldStatus = {};
    for (int i = 0; i < 4; ++i) {
        layout.filters[i] = nodeBounds(*root, ("filter" + std::to_string(i)).c_str());
    }
    return layout;
}


Rectangle tabBounds(Rectangle bounds, int index, int count)
{
    auto root = ui::grid(std::max(1, count), "tabs");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = 0.0f;
    root->style(rootStyle);
    root->fixedCellHeight = bounds.height;

    for (int i = 0; i < std::max(1, count); ++i) {
        auto tab = std::make_unique<ui::PanelNode>("tab" + std::to_string(i));
        tab->style(ui::fixedHeight(bounds.height));
        root->add(std::move(tab));
    }

    root->measure({bounds.width, bounds.height});
    root->layout(bounds);
    return nodeBounds(*root, ("tab" + std::to_string(std::clamp(index, 0, std::max(1, count) - 1))).c_str());
}

std::string plannedLabel(const UiState& state)
{
    std::string label;
    int shown = 0;
    for (const auto& planned : state.plannedInterventions) {
        if (shown >= 2) {
            break;
        }
        if (!label.empty()) {
            label += ", ";
        }
        label += planned.actionName;
        ++shown;
    }
    if (state.plannedInterventions.size() > static_cast<std::size_t>(shown)) {
        label += ", +" + std::to_string(state.plannedInterventions.size() - static_cast<std::size_t>(shown));
    }
    return label;
}

void drawFilterPill(Rectangle bounds, const std::string& label, bool active)
{
    DrawRectangleRounded(bounds, 0.22f, 8, active ? Color{97, 64, 196, 245} : Color{28, 36, 48, 210});
    DrawRectangleRoundedLines(bounds, 0.22f, 8, active ? Color{145, 109, 255, 210} : Color{70, 86, 104, 90});
    drawTextClipped(label, {bounds.x + 10.0f, bounds.y + 5.0f, bounds.width - 20.0f, 15.0f}, 12, active ? Color{244, 240, 255, 255} : Color{166, 176, 192, 255});
}

void drawMetricBar(Rectangle bounds, const char* label, double value, Color color)
{
    drawTextClipped(label, {bounds.x, bounds.y, 92.0f, 16.0f}, 13, {185, 195, 210, 255});
    DrawRectangleRounded({bounds.x + 102.0f, bounds.y + 5.0f, bounds.width - 154.0f, 5.0f}, 0.5f, 6, {45, 55, 68, 255});
    DrawRectangleRounded({bounds.x + 102.0f, bounds.y + 5.0f, (bounds.width - 154.0f) * static_cast<float>(std::clamp(value, 0.0, 1.0)), 5.0f}, 0.5f, 6, color);
    char text[24];
    std::snprintf(text, sizeof(text), "%.0f%%", value * 100.0);
    DrawText(text, static_cast<int>(bounds.x + bounds.width - 42.0f), static_cast<int>(bounds.y - 1.0f), 13, {230, 237, 243, 255});
}
}

void ActionPanel::update(UiContext& context, const Simulation& simulation)
{
    if (context.state == nullptr) {
        return;
    }
    context.state->hoveredActionIndex = -1;
    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);
    const Vector2 mouse = GetMousePosition();
    context.state->hoveredWorldActionIndex = -1;
    if (context.state->eventPopupMode != EventPopupMode::None) {
        return;
    }
    if (context.state->gameplayPhase == GameplayPhase::Planning && context.state->worldActionDraftVisible) {
        const Rectangle overlay = WorldActionOverlay::overlayBounds(context.screenWidth, context.screenHeight);
        const int count = static_cast<int>(context.state->worldActionDraft.size());
        for (int i = 0; i < static_cast<int>(context.state->worldActionDraft.size()); ++i) {
            if (CheckCollisionPointRec(mouse, WorldActionOverlay::draftCardBounds(overlay, i, count))) {
                context.state->hoveredWorldActionIndex = i;
                return;
            }
        }
    }
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        if (CheckCollisionPointRec(mouse, cards[static_cast<std::size_t>(i)].bounds)) {
            context.state->hoveredActionIndex = i;
            return;
        }
    }
}

void ActionPanel::draw(const UiContext& context, const Simulation& simulation) const
{
    if (context.state == nullptr) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const Rectangle sidebar = layout.rightSidebar;
    const RightSidebarLayout panel = computeRightSidebarLayout(sidebar);
    DrawRectangleRounded(sidebar, 0.018f, 8, {9, 16, 27, 242});
    DrawRectangleRoundedLines(sidebar, 0.018f, 8, {62, 82, 112, 130});

    const Node* selected = simulation.graph().node(context.state->selection.nodeId);
    const NodePressure* selectedPressure = selected != nullptr ? simulation.pressureAnalysis().pressureForNode(selected->id) : nullptr;
    const Rectangle header = panel.header;
    if (selected != nullptr) {
        DrawRectangleRounded({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {37, 50, 82, 255});
        DrawRectangleRoundedLines({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {130, 93, 255, 220});
        IconRegistry::instance().drawIcon("node.header_selected", {header.x + 14.0f, header.y + 18.0f, 26.0f, 26.0f}, {89, 196, 255, 255});
        drawTextClipped(selected->name, {header.x + 70.0f, header.y + 4.0f, header.width - 150.0f, 30.0f}, 25, {241, 245, 249, 255});
        const char* region = selected->hasGeoLocation ? selected->geoLocation.regionName.c_str() : "local";
        drawTextClipped(region, {header.x + 70.0f, header.y + 40.0f, 140.0f, 20.0f}, 14, {166, 176, 192, 255});
        const double load = actions_ui::overallPressure(selectedPressure, selected);
        const char* status = load >= 0.72 ? "Under heavy load" : load >= 0.42 ? "Moderate pressure" : "Stable";
        DrawCircle(static_cast<int>(header.x + header.width - 126.0f), static_cast<int>(header.y + 17.0f), 6.0f, actions_ui::pressureColor(load));
        drawTextClipped(status, {header.x + header.width - 112.0f, header.y + 6.0f, 108.0f, 18.0f}, 14, actions_ui::pressureColor(load));
    } else {
        DrawRectangleRounded({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {37, 50, 82, 255});
        IconRegistry::instance().drawIcon("node.header_empty", {header.x + 14.0f, header.y + 18.0f, 26.0f, 26.0f}, {139, 148, 158, 255});
        drawTextClipped("Select a Node", {header.x + 70.0f, header.y + 6.0f, header.width - 84.0f, 30.0f}, 25, {241, 245, 249, 255});
        drawTextClipped("Inspect local actions and pressures", {header.x + 70.0f, header.y + 42.0f, header.width - 84.0f, 20.0f}, 14, {166, 176, 192, 255});
    }

    DrawLine(static_cast<int>(sidebar.x), static_cast<int>(panel.tabs.y - 5.0f), static_cast<int>(sidebar.x + sidebar.width), static_cast<int>(panel.tabs.y - 5.0f), {31, 42, 58, 255});
    const char* tabs[] = {"Overview", "Metrics", "Traffic", "Dependencies"};
    for (int i = 0; i < 4; ++i) {
        const Rectangle tab = tabBounds(panel.tabs, i, 4);
        drawTextClipped(tabs[i], {tab.x + 4.0f, tab.y + 10.0f, tab.width - 8.0f, 18.0f}, 13, i == 0 ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
        if (i == 0) {
            DrawRectangleRounded({tab.x, tab.y + tab.height - 4.0f, tab.width - 10.0f, 3.0f}, 0.4f, 6, {145, 109, 255, 255});
        }
    }

    const Rectangle overview = panel.overview;
    const Rectangle description{overview.x, overview.y, overview.width * 0.58f - 7.0f, overview.height};
    const Rectangle status{description.x + description.width + 14.0f, overview.y, overview.width - description.width - 14.0f, overview.height};
    DrawRectangleRounded(description, 0.035f, 8, {13, 34, 47, 226});
    DrawRectangleRoundedLines(description, 0.035f, 8, {48, 95, 112, 115});
    DrawText("OVERVIEW", static_cast<int>(description.x + 14.0f), static_cast<int>(description.y + 15.0f), 12, {166, 176, 192, 255});
    if (selected != nullptr) {
        const auto& nodeDef = NodeRegistry::definition(selected->type);
        actions_ui::drawWrappedTextClipped(std::string("This ") + std::string(nodeDef.displayName) + " handles local request flow and participates in the dependency path.", {description.x + 16.0f, description.y + 44.0f, description.width - 32.0f, 68.0f}, 13, {205, 213, 224, 255});
        actions_ui::drawWrappedTextClipped(selectedPressure != nullptr && !selectedPressure->explanation.empty() ? selectedPressure->explanation : "Watch traffic, queue depth, and dependency pressure before committing changes.", {description.x + 16.0f, description.y + 122.0f, description.width - 32.0f, description.height - 134.0f}, 12, {166, 176, 192, 255});
    } else {
        actions_ui::drawWrappedTextClipped("Select an infrastructure node to see its role, health, pressures, and contextual actions.", {description.x + 16.0f, description.y + 44.0f, description.width - 32.0f, description.height - 58.0f}, 13, {205, 213, 224, 255});
    }

    DrawRectangleRounded(status, 0.035f, 8, {18, 24, 34, 230});
    DrawRectangleRoundedLines(status, 0.035f, 8, {70, 86, 104, 110});
    const double overall = actions_ui::overallPressure(selectedPressure, selected);
    drawTextClipped("HEALTH", {status.x + 14.0f, status.y + 15.0f, 80.0f, 16.0f}, 14, {205, 213, 224, 255});
    drawTextClipped(actions_ui::pressureSeverity(overall), {status.x + status.width - 78.0f, status.y + 15.0f, 64.0f, 16.0f}, 13, actions_ui::pressureColor(overall));
    drawMetricBar({status.x + 14.0f, status.y + 48.0f, status.width - 28.0f, 18.0f}, "Utilization", selected != nullptr ? selected->currentUtilization : 0.0, actions_ui::pressureColor(selected != nullptr ? selected->currentUtilization : 0.0));
    drawMetricBar({status.x + 14.0f, status.y + 78.0f, status.width - 28.0f, 18.0f}, "Queue", selectedPressure != nullptr ? selectedPressure->queuePressure : 0.0, actions_ui::pressureColor(selectedPressure != nullptr ? selectedPressure->queuePressure : 0.0));
    drawMetricBar({status.x + 14.0f, status.y + 108.0f, status.width - 28.0f, 18.0f}, "Latency", selectedPressure != nullptr ? selectedPressure->latencyContribution : 0.0, actions_ui::pressureColor(selectedPressure != nullptr ? selectedPressure->latencyContribution : 0.0));
    DrawText("PRIMARY PRESSURES", static_cast<int>(status.x + 14.0f), static_cast<int>(status.y + 138.0f), 11, {166, 176, 192, 255});
    const PressureCategory pressures[] = {selectedPressure != nullptr ? selectedPressure->dominant : PressureCategory::None, PressureCategory::QueuePressure, PressureCategory::PersistencePressure};
    const double pressureValues[] = {overall, selectedPressure != nullptr ? selectedPressure->queuePressure : 0.0, selectedPressure != nullptr ? selectedPressure->dependencyPressure : 0.0};
    for (int i = 0; i < 3; ++i) {
        const float y = status.y + 156.0f + static_cast<float>(i) * 16.0f;
        DrawCircle(static_cast<int>(status.x + 18.0f), static_cast<int>(y + 6.0f), 4.0f, actions_ui::pressureColor(pressureValues[i]));
        drawTextClipped(pressureCategoryName(pressures[i]), {status.x + 28.0f, y, status.width - 100.0f, 12.0f}, 11, {230, 237, 243, 255});
        drawTextClipped(actions_ui::pressureSeverity(pressureValues[i]), {status.x + status.width - 66.0f, y, 52.0f, 12.0f}, 11, actions_ui::pressureColor(pressureValues[i]));
    }

    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);
    const ActionSectionsLayout actions = model.actionSectionsLayout(*context.state, context.screenWidth, context.screenHeight);
    DrawText("AVAILABLE NODE ACTIONS", static_cast<int>(actions.title.x), static_cast<int>(actions.title.y), 14, {230, 237, 243, 255});
    engineeringCapacityPanel_.draw(actions.capacity, *context.state);
    for (int i = 0; i < 4; ++i) {
        drawFilterPill(actions.filters[i], actions_ui::categoryFilterLabel(cards, i), i == 0);
    }
    BeginScissorMode(static_cast<int>(actions.actionList.x - 2.0f), static_cast<int>(actions.actionList.y), static_cast<int>(actions.actionList.width + 4.0f), static_cast<int>(actions.actionList.height));
    if (cards.empty()) {
        drawTextClipped("Select an API, database, cache, or queue node to see contextual actions.", {actions.actionList.x, actions.actionList.y + 4.0f, actions.actionList.width, 18.0f}, 13, {139, 148, 158, 255});
    }
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        const auto& card = cards[static_cast<std::size_t>(i)];
        if (!card.available || card.bounds.height <= 0.0f) {
            continue;
        }
        const bool highlighted = i == context.state->hoveredActionIndex || i == context.state->selectedActionIndex;
        NodeActionCardView{}.draw(card, highlighted);
    }
    EndScissorMode();

    const Rectangle locked = panel.locked;
    DrawRectangleRounded(locked, 0.045f, 8, {17, 24, 34, 230});
    DrawRectangleRoundedLines(locked, 0.045f, 8, {70, 86, 104, 110});
    IconRegistry::instance().drawIcon("action.locked", {locked.x + 16.0f, locked.y + 24.0f, 24.0f, 24.0f}, {139, 148, 158, 180});
    int lockedCount = 0;
    std::string lockedReason = "More actions will be discovered as scenarios unlock concepts.";
    for (const auto& card : cards) {
        if (!card.available) {
            ++lockedCount;
            if (!card.unavailableReason.empty()) {
                lockedReason = card.unavailableReason;
            }
        }
    }
    drawTextClipped(lockedCount > 0 ? std::to_string(lockedCount) + " locked action(s)" : "Locked actions", {locked.x + 56.0f, locked.y + 17.0f, locked.width - 72.0f, 20.0f}, 13, {166, 176, 192, 255});
    drawTextClipped(lockedReason, {locked.x + 56.0f, locked.y + 42.0f, locked.width - 72.0f, 18.0f}, 12, {139, 148, 158, 255});

    if (context.state->placementActive) {
        const PlacementCandidateGenerator generator;
        const auto candidates = generator.generate(simulation, context.state->activeMutation);
        if (!candidates.empty()) {
            const int index = std::clamp(context.state->placementCandidateIndex, 0, static_cast<int>(candidates.size()) - 1);
            drawTextClipped("Placement: " + candidates[static_cast<std::size_t>(index)].displayName, panel.statusMessage, 12, {89, 196, 255, 255});
        }
    }

    DrawRectangleRounded(panel.button, 0.08f, 8, {97, 64, 196, static_cast<unsigned char>(context.state->placementActive ? 255 : 150)});
    const bool hasSelectedAction = context.state->selectedActionIndex >= 0;
    DrawText(context.state->placementActive ? "Queue Placement" : (hasSelectedAction ? "Queue Action" : "Select an Action"), static_cast<int>(panel.button.x + panel.button.width * 0.5f - 58.0f), static_cast<int>(panel.button.y + 14.0f), 15, {230, 237, 243, 255});

    if (!context.state->latestFeedback.empty() && !context.state->placementActive) {
        drawTextClipped(context.state->latestFeedback, panel.statusMessage, 13, {245, 184, 76, 255});
    }
    if (!context.state->plannedInterventions.empty()) {
        drawTextClipped("Planned: " + plannedLabel(*context.state), panel.summaryMessage, 12, {86, 210, 151, 255});
    } else if (context.state->selectedWorldActionIndex >= 0 && context.state->selectedWorldActionIndex < static_cast<int>(context.state->worldActionDraft.size())) {
        drawTextClipped("World action: " + context.state->worldActionDraft[static_cast<std::size_t>(context.state->selectedWorldActionIndex)].name, panel.summaryMessage, 12, {86, 210, 151, 255});
    } else if (!context.state->resolutionSummaries.empty()) {
        drawTextClipped(context.state->resolutionSummaries.front(), panel.summaryMessage, 12, {86, 210, 151, 255});
    }
}

void ActionPanel::drawPlanningOverlays(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    eventOverlay_.draw(context, scenarioManager);
    worldActionOverlay_.draw(context);
}
