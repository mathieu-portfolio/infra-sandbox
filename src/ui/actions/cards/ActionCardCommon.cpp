#include "ui/actions/cards/ActionCardCommon.hpp"

#include "ui/core/UiPrimitives.hpp"

#include <string>

namespace actions_ui::cards {

const char* iconForActionCard(const ActionCardModel& card)
{
    if (!card.iconId.empty()) {
        return card.iconId.c_str();
    }
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
    return "action.default";
}

std::string categoryLabel(const ActionCardModel& card)
{
    if (!card.categories.empty()) {
        return card.categories.front();
    }
    if (!card.affectedPressures.empty()) {
        return pressureCategoryName(card.affectedPressures.front());
    }
    return "Local";
}

int actionPointCost(const std::vector<EngineeringCost>& costs)
{
    int total = 0;
    for (const auto& cost : costs) {
        total += cost.amount;
    }
    return total;
}

std::vector<std::string> usefulPoints(const ActionCardModel& card)
{
    if (!card.usefulWhen.empty()) {
        return card.usefulWhen;
    }
    if (!card.positiveEffects.empty()) {
        return card.positiveEffects;
    }
    return {"local pressure matches this action"};
}

std::vector<std::string> worsenPoints(const ActionCardModel& card)
{
    if (!card.negativeEffects.empty()) {
        return card.negativeEffects;
    }
    if (!card.tradeOff.empty()) {
        return {card.tradeOff};
    }
    return {"adds operational complexity"};
}

void drawCardChrome(Rectangle bounds, CardVisualState state)
{
    const bool active = state.highlighted || state.selected || state.hovered;
    const Color border = active ? Color{145, 109, 255, 230} : Color{74, 92, 120, 130};
    const Color fill = active ? Color{36, 32, 58, 238} : Color{23, 30, 42, 238};
    DrawRectangleRounded(bounds, 0.045f, 8, fill);
    DrawRectangleRoundedLines(bounds, 0.045f, 8, border);
}

void drawChip(Rectangle bounds, const std::string& label, Color fill, Color text)
{
    DrawRectangleRounded(bounds, 0.2f, 6, fill);
    drawTextClipped(label, {bounds.x + 9.0f, bounds.y + 4.0f, bounds.width - 18.0f, 14.0f}, 10, text);
}

}
