#include "ui/InterventionPanel.hpp"

#include "ui/ActionPanelModel.hpp"
#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"
#include "ui/UiPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>

namespace {
void section(Rectangle bounds, const char* title, int step)
{
    DrawRectangleRounded(bounds, 0.04f, 8, {22, 27, 34, 224});
    DrawRectangleRoundedLines(bounds, 0.04f, 8, {70, 86, 104, 95});
    char label[96];
    std::snprintf(label, sizeof(label), "%d.  %s", step, title);
    drawTextClipped(label, {bounds.x + 12.0f, bounds.y + 10.0f, bounds.width - 24.0f, 20.0f}, 16, {89, 196, 255, 255});
}

Color availableText(bool available)
{
    return available ? Color{230, 237, 243, 255} : Color{139, 148, 158, 170};
}

const char* iconForAction(const ActionCard& card)
{
    if (card.name.find("Scale") != std::string::npos) {
        return "action.scale_up";
    }
    if (card.name.find("Cache") != std::string::npos) {
        return "action.add_cache";
    }
    if (card.name.find("Replica") != std::string::npos) {
        return "action.replica";
    }
    if (card.name.find("Queue") != std::string::npos) {
        return "action.queue";
    }
    return "action.generic";
}
}

void InterventionPanel::update(UiContext& context, const Simulation& simulation)
{
    if (context.state == nullptr) {
        return;
    }
    context.state->hoveredActionIndex = -1;
    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);
    const Vector2 mouse = GetMousePosition();
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        if (CheckCollisionPointRec(mouse, cards[static_cast<std::size_t>(i)].bounds)) {
            context.state->hoveredActionIndex = i;
            return;
        }
    }
}

void InterventionPanel::draw(const UiContext& context, const Simulation& simulation) const
{
    if (context.state == nullptr) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const Rectangle sidebar = layout.rightSidebar;
    DrawRectangleRounded(sidebar, 0.025f, 8, {13, 17, 23, 225});
    DrawRectangleRoundedLines(sidebar, 0.025f, 8, {70, 86, 104, 100});

    const Rectangle target{sidebar.x + 10.0f, sidebar.y + 10.0f, sidebar.width - 20.0f, 198.0f};
    section(target, "SELECT A TARGET", 1);
    const Node* selected = simulation.graph().node(context.state->selection.nodeId);
    if (selected != nullptr) {
        IconRegistry::instance().drawIcon("node.service", {target.x + 14.0f, target.y + 44.0f, 24.0f, 24.0f}, {89, 196, 255, 255});
        drawTextClipped(selected->name, {target.x + 46.0f, target.y + 42.0f, target.width - 66.0f, 20.0f}, 16, {230, 237, 243, 255});
        const char* region = selected->hasGeoLocation ? selected->geoLocation.regionName.c_str() : "local";
        drawTextClipped(region, {target.x + 46.0f, target.y + 62.0f, target.width - 66.0f, 16.0f}, 13, {139, 148, 158, 255});
        char buffer[120];
        std::snprintf(buffer, sizeof(buffer), "Utilization %.0f%%", selected->currentUtilization * 100.0);
        DrawText(buffer, static_cast<int>(target.x + 14.0f), static_cast<int>(target.y + 96.0f), 14, {230, 237, 243, 255});
        std::snprintf(buffer, sizeof(buffer), "Queue Depth %zu", selected->queue.size());
        DrawText(buffer, static_cast<int>(target.x + 14.0f), static_cast<int>(target.y + 118.0f), 14, {230, 237, 243, 255});
        std::snprintf(buffer, sizeof(buffer), "Wait %.2fs", selected->averageQueueWaitSeconds);
        DrawText(buffer, static_cast<int>(target.x + 14.0f), static_cast<int>(target.y + 140.0f), 14, {230, 237, 243, 255});
        if (selected->type == NodeType::ApiService) {
            std::snprintf(buffer, sizeof(buffer), "Scale Level %d/%d", selected->scaleLevel, selected->maxScaleLevel);
            DrawText(buffer, static_cast<int>(target.x + 14.0f), static_cast<int>(target.y + 162.0f), 14, {89, 196, 255, 255});
        } else if (selected->hasGeoLocation) {
            const int used = simulation.regionSlotsUsed(selected->geoLocation.regionName);
            const int limit = simulation.regionSlotLimit(selected->geoLocation.regionName);
            std::snprintf(buffer, sizeof(buffer), "%s Slots %d/%d", selected->geoLocation.regionName.c_str(), used, limit);
            DrawText(buffer, static_cast<int>(target.x + 14.0f), static_cast<int>(target.y + 162.0f), 14, {89, 196, 255, 255});
        }
    } else {
        drawTextClipped("Click a node or choose a global action.", {target.x + 14.0f, target.y + 48.0f, target.width - 28.0f, 20.0f}, 14, {139, 148, 158, 255});
    }

    const float buttonY = sidebar.y + sidebar.height - 54.0f;
    const Rectangle preview{sidebar.x + 10.0f, buttonY - UiTheme::gap - 170.0f, sidebar.width - 20.0f, 170.0f};
    const Rectangle actions{sidebar.x + 10.0f, target.y + target.height + UiTheme::gap, sidebar.width - 20.0f, preview.y - (target.y + target.height + UiTheme::gap) - UiTheme::gap};
    section(actions, "CHOOSE AN ACTION", 2);
    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);
    const int maxCards = std::min(static_cast<int>(cards.size()), std::max(0, static_cast<int>((actions.height - 44.0f) / 62.0f)));
    BeginScissorMode(static_cast<int>(actions.x), static_cast<int>(actions.y), static_cast<int>(actions.width), static_cast<int>(actions.height));
    for (int i = 0; i < maxCards; ++i) {
        const auto& card = cards[static_cast<std::size_t>(i)];
        const bool highlighted = i == context.state->hoveredActionIndex || i == context.state->selectedActionIndex;
        const Color border = highlighted ? Color{37, 120, 255, 240} : Color{70, 86, 104, 115};
        DrawRectangleRounded(card.bounds, 0.06f, 6, card.available ? Color{30, 36, 44, 235} : Color{24, 28, 35, 180});
        DrawRectangleRoundedLines(card.bounds, 0.06f, 6, border);
        IconRegistry::instance().drawIcon(iconForAction(card), {card.bounds.x + 10.0f, card.bounds.y + 10.0f, 24.0f, 24.0f}, card.available ? Color{89, 196, 255, 255} : Color{139, 148, 158, 160});
        drawTextClipped(card.name, {card.bounds.x + 42.0f, card.bounds.y + 8.0f, card.bounds.width - 52.0f, 18.0f}, 15, availableText(card.available));
        if (!card.stateLabel.empty()) {
            drawTextClipped(card.stateLabel, {card.bounds.x + card.bounds.width - 88.0f, card.bounds.y + 8.0f, 78.0f, 15.0f}, 11, card.available ? Color{86, 210, 151, 220} : Color{235, 86, 100, 220});
        }
        drawTextClipped(card.description, {card.bounds.x + 42.0f, card.bounds.y + 28.0f, card.bounds.width - 52.0f, 15.0f}, 12, {139, 148, 158, 255});
        drawTextClipped(card.available ? card.helps : card.unavailableReason, {card.bounds.x + 42.0f, card.bounds.y + 44.0f, card.bounds.width - 52.0f, 15.0f}, 12, card.available ? Color{86, 210, 151, 220} : Color{235, 86, 100, 220});
    }
    EndScissorMode();

    section(preview, "PREVIEW IMPACT", 3);
    const int selectedIndex = context.state->hoveredActionIndex >= 0 ? context.state->hoveredActionIndex : context.state->selectedActionIndex;
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(cards.size())) {
        const auto& card = cards[static_cast<std::size_t>(selectedIndex)];
        drawTextClipped(card.name, {preview.x + 14.0f, preview.y + 42.0f, preview.width - 28.0f, 20.0f}, 16, {230, 237, 243, 255});
        const std::string helps = !card.positiveEffects.empty() ? card.positiveEffects.front() : card.helps;
        const std::string downside = !card.negativeEffects.empty() ? card.negativeEffects.front() : card.tradeOff;
        const std::string shift = !card.pressureShifts.empty() ? card.pressureShifts.front() : "Watch pressure after applying.";
        drawTextClipped("+ " + helps, {preview.x + 14.0f, preview.y + 68.0f, preview.width - 28.0f, 17.0f}, 13, {86, 210, 151, 255});
        drawTextClipped("- " + downside, {preview.x + 14.0f, preview.y + 88.0f, preview.width - 28.0f, 17.0f}, 13, {245, 184, 76, 255});
        drawTextClipped("Shift: " + shift, {preview.x + 14.0f, preview.y + 108.0f, preview.width - 28.0f, 17.0f}, 13, {89, 196, 255, 255});
        char impact[120];
        if (card.maxScaleLevel > 0) {
            std::snprintf(impact, sizeof(impact), "Scale %d/%d  Complexity +%.1f", card.currentScaleLevel, card.maxScaleLevel, card.complexityCost);
        } else if (card.regionSlotUsage > 0) {
            std::snprintf(impact, sizeof(impact), "Uses %d regional slot  Complexity +%.1f", card.regionSlotUsage, card.complexityCost);
        } else {
            std::snprintf(impact, sizeof(impact), "Complexity +%.1f", card.complexityCost);
        }
        drawTextClipped(impact, {preview.x + 14.0f, preview.y + 128.0f, preview.width - 28.0f, 16.0f}, 12, {139, 148, 158, 255});
    } else {
        drawTextClipped("Hover or select an action to inspect effects.", {preview.x + 14.0f, preview.y + 46.0f, preview.width - 28.0f, 18.0f}, 14, {139, 148, 158, 255});
    }

    if (context.state->placementActive) {
        const PlacementCandidateGenerator generator;
        const auto candidates = generator.generate(simulation, context.state->activeMutation);
        if (!candidates.empty()) {
            const int index = std::clamp(context.state->placementCandidateIndex, 0, static_cast<int>(candidates.size()) - 1);
            DrawRectangleRounded({preview.x + 14.0f, preview.y + 116.0f, 28.0f, 24.0f}, 0.2f, 6, {30, 36, 44, 230});
            DrawText("<", static_cast<int>(preview.x + 23.0f), static_cast<int>(preview.y + 120.0f), 16, {230, 237, 243, 255});
            DrawRectangleRounded({preview.x + preview.width - 42.0f, preview.y + 116.0f, 28.0f, 24.0f}, 0.2f, 6, {30, 36, 44, 230});
            DrawText(">", static_cast<int>(preview.x + preview.width - 33.0f), static_cast<int>(preview.y + 120.0f), 16, {230, 237, 243, 255});
            drawTextClipped(candidates[static_cast<std::size_t>(index)].displayName, {preview.x + 48.0f, preview.y + 120.0f, preview.width - 96.0f, 18.0f}, 14, {89, 196, 255, 255});
            drawTextClipped("Use arrows to choose a region, then apply below.", {preview.x + 14.0f, preview.y + 142.0f, preview.width - 28.0f, 15.0f}, 12, {139, 148, 158, 255});
        }
    }

    DrawRectangleRounded({sidebar.x + 12.0f, buttonY, sidebar.width - 24.0f, 40.0f}, 0.08f, 8, {37, 120, 255, static_cast<unsigned char>(context.state->placementActive ? 255 : 120)});
    const bool hasSelectedAction = context.state->selectedActionIndex >= 0;
    DrawText(context.state->placementActive ? "Apply Action" : (hasSelectedAction ? "Apply Selected Action" : "Select an Action"), static_cast<int>(sidebar.x + sidebar.width * 0.5f - 76.0f), static_cast<int>(buttonY + 12.0f), 15, {230, 237, 243, 255});

    if (!context.state->latestFeedback.empty()) {
        drawTextClipped(context.state->latestFeedback, {sidebar.x + 12.0f, buttonY - 24.0f, sidebar.width - 24.0f, 18.0f}, 13, {245, 184, 76, 255});
    }
}
