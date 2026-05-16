#include "ui/InterventionPanel.hpp"

#include "ui/ActionPanelModel.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>

namespace {
Color textColor(bool available)
{
    return available ? Color{230, 237, 243, 255} : Color{139, 148, 158, 170};
}

void drawWrappedLine(const std::string& text, int x, int y, int size, Color color)
{
    DrawText(text.c_str(), x, y, size, color);
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

    const ActionPanelModel model;
    const Rectangle panel = model.panelBounds(context.screenWidth, context.screenHeight);
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);

    DrawRectangleRounded(panel, 0.035f, 8, {22, 27, 34, 232});
    DrawText("Actions", static_cast<int>(panel.x + 12.0f), static_cast<int>(panel.y + 10.0f), 18, {230, 237, 243, 255});

    const int maxCards = std::min(static_cast<int>(cards.size()), 7);
    for (int i = 0; i < maxCards; ++i) {
        const auto& card = cards[static_cast<std::size_t>(i)];
        const bool highlighted = i == context.state->hoveredActionIndex || i == context.state->selectedActionIndex;
        const Color border = highlighted ? Color{89, 196, 255, 200} : Color{70, 86, 104, 130};
        const Color fill = card.available ? Color{30, 36, 44, 230} : Color{24, 28, 35, 180};
        DrawRectangleRounded(card.bounds, 0.06f, 6, fill);
        DrawRectangleRoundedLines(card.bounds, 0.06f, 6, border);
        DrawText(card.name.c_str(), static_cast<int>(card.bounds.x + 10.0f), static_cast<int>(card.bounds.y + 7.0f), 16, textColor(card.available));
        DrawText(card.target.c_str(), static_cast<int>(card.bounds.x + card.bounds.width - MeasureText(card.target.c_str(), 13) - 10.0f), static_cast<int>(card.bounds.y + 9.0f), 13, {139, 148, 158, 210});
        drawWrappedLine(("Helps: " + card.helps), static_cast<int>(card.bounds.x + 10.0f), static_cast<int>(card.bounds.y + 29.0f), 13, textColor(card.available));
        const std::string bottom = card.available ? ("Trade-off: " + card.tradeOff) : card.unavailableReason;
        drawWrappedLine(bottom, static_cast<int>(card.bounds.x + 10.0f), static_cast<int>(card.bounds.y + 47.0f), 12, card.available ? Color{245, 184, 76, 220} : Color{235, 86, 100, 220});
    }

    if (context.state->placementActive) {
        const PlacementCandidateGenerator generator;
        const MutationValidator validator;
        const auto candidates = generator.generate(simulation, context.state->activeMutation);
        if (!candidates.empty()) {
            const int index = std::clamp(context.state->placementCandidateIndex, 0, static_cast<int>(candidates.size()) - 1);
            const auto& option = candidates[static_cast<std::size_t>(index)];
            const MutationPreview preview = validator.preview(simulation, context.state->activeMutation, option);
            const int y = static_cast<int>(panel.y + panel.height - 138.0f);
            DrawText("Placement Preview", static_cast<int>(panel.x + 12.0f), y, 16, {89, 196, 255, 255});
            char buffer[180];
            std::snprintf(buffer, sizeof(buffer), "%d/%zu  %s", index + 1, candidates.size(), option.displayName.c_str());
            DrawText(buffer, static_cast<int>(panel.x + 12.0f), y + 22, 15, {230, 237, 243, 255});
            DrawText(option.latencyImpact.c_str(), static_cast<int>(panel.x + 12.0f), y + 44, 14, {139, 148, 158, 255});
            DrawText(option.trafficImpact.c_str(), static_cast<int>(panel.x + 12.0f), y + 64, 14, {139, 148, 158, 255});
            DrawText(preview.validationMessage.c_str(), static_cast<int>(panel.x + 12.0f), y + 86, 14, preview.valid ? Color{86, 210, 151, 255} : Color{235, 86, 100, 255});
            DrawText("Left/Right choose | Enter confirm | Backspace cancel", static_cast<int>(panel.x + 12.0f), y + 108, 13, {139, 148, 158, 255});
        }
    }

    if (!context.state->latestFeedback.empty()) {
        DrawText("Latest feedback", static_cast<int>(panel.x + 12.0f), static_cast<int>(panel.y + panel.height - 42.0f), 14, {245, 184, 76, 255});
        DrawText(context.state->latestFeedback.c_str(), static_cast<int>(panel.x + 12.0f), static_cast<int>(panel.y + panel.height - 22.0f), 13, {230, 237, 243, 255});
    }
}
