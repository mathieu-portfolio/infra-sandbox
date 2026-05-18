#include "ui/actions/cards/WorldActionCardView.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiPrimitives.hpp"
#include "ui/actions/ActionText.hpp"

#include <string>

namespace {
std::string capacityBonusLabel(const EngineeringCapacity& bonus, bool verbose)
{
    std::string text;
    if (bonus.backend > 0) text += std::string(verbose ? "Backend" : "Back") + " +" + std::to_string(bonus.backend) + " ";
    if (bonus.infrastructure > 0) text += "Infra +" + std::to_string(bonus.infrastructure) + " ";
    if (bonus.operations > 0) text += "Ops +" + std::to_string(bonus.operations) + " ";
    if (bonus.data > 0) text += "Data +" + std::to_string(bonus.data) + " ";
    if (verbose && bonus.frontend > 0) text += "Frontend +" + std::to_string(bonus.frontend) + " ";
    if (text.empty() && bonus.total > 0) text = "Total +" + std::to_string(bonus.total);
    return text;
}
}

void WorldActionCardView::drawCompact(Rectangle bounds, const WorldActionDraft& action, bool selected, bool hovered) const
{
    const Color fill = selected ? Color{42, 37, 68, 245} : hovered ? Color{29, 38, 54, 240} : Color{18, 24, 34, 230};
    const Color border = selected ? Color{145, 109, 255, 230} : Color{70, 86, 104, 120};
    DrawRectangleRounded(bounds, 0.07f, 8, fill);
    DrawRectangleRoundedLines(bounds, 0.07f, 8, border);
    IconRegistry::instance().drawIcon(action.iconId.empty() ? "action.generic" : action.iconId.c_str(), {bounds.x + 9.0f, bounds.y + 10.0f, 18.0f, 18.0f}, {89, 196, 255, 255});
    drawTextClipped(action.name, {bounds.x + 33.0f, bounds.y + 8.0f, bounds.width - 42.0f, 18.0f}, 11, {230, 237, 243, 255});
    drawTextClipped(action.category, {bounds.x + 10.0f, bounds.y + 32.0f, bounds.width - 20.0f, 14.0f}, 10, {189, 135, 255, 255});
    const std::string bonus = capacityBonusLabel(action.capacityBonus, false);
    drawTextClipped(bonus.empty() ? "Strategic focus" : bonus, {bounds.x + 10.0f, bounds.y + 55.0f, bounds.width - 20.0f, 14.0f}, 10, {86, 210, 151, 255});
}

void WorldActionCardView::drawDraft(Rectangle bounds, const WorldActionDraft& action, bool selected, bool hovered) const
{
    const Color fill = selected ? Color{41, 35, 65, 250} : hovered ? Color{24, 34, 50, 250} : Color{15, 22, 33, 246};
    const Color border = selected ? Color{145, 109, 255, 235} : hovered ? Color{89, 196, 255, 190} : Color{70, 86, 104, 130};
    DrawRectangleRounded(bounds, 0.035f, 8, fill);
    DrawRectangleRoundedLines(bounds, 0.035f, 8, border);
    IconRegistry::instance().drawIcon(action.iconId.empty() ? "action.generic" : action.iconId.c_str(), {bounds.x + 16.0f, bounds.y + 16.0f, 28.0f, 28.0f}, {89, 196, 255, 255});
    drawTextClipped(action.name, {bounds.x + 54.0f, bounds.y + 14.0f, bounds.width - 70.0f, 24.0f}, 18, {241, 245, 249, 255});
    drawTextClipped(action.category, {bounds.x + 54.0f, bounds.y + 41.0f, bounds.width - 70.0f, 18.0f}, 12, {189, 135, 255, 255});
    actions_ui::drawWrappedTextClipped(action.description, {bounds.x + 16.0f, bounds.y + 74.0f, bounds.width - 32.0f, 54.0f}, 13, {205, 213, 224, 255});
    drawTextClipped("Useful when", {bounds.x + 16.0f, bounds.y + 138.0f, bounds.width - 32.0f, 16.0f}, 11, {86, 210, 151, 255});
    actions_ui::drawWrappedTextClipped(action.usefulWhen.empty() ? "the current pressure pattern matches this strategic focus" : action.usefulWhen, {bounds.x + 16.0f, bounds.y + 156.0f, bounds.width - 32.0f, 38.0f}, 12, {185, 195, 210, 255});
    drawTextClipped("May worsen", {bounds.x + 16.0f, bounds.y + 202.0f, bounds.width - 32.0f, 16.0f}, 11, {245, 184, 76, 255});
    actions_ui::drawWrappedTextClipped(action.tradeOff.empty() ? "coordination complexity" : action.tradeOff, {bounds.x + 16.0f, bounds.y + 220.0f, bounds.width - 32.0f, 38.0f}, 12, {185, 195, 210, 255});
    const std::string bonus = capacityBonusLabel(action.capacityBonus, true);
    drawTextClipped(bonus.empty() ? "No capacity change" : bonus, {bounds.x + 16.0f, bounds.y + bounds.height - 30.0f, bounds.width - 32.0f, 18.0f}, 12, {86, 210, 151, 255});
}
