#include "ui/actions/cards/WorldActionCardView.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiPrimitives.hpp"
#include "ui/actions/ActionText.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace {
constexpr float kDraftPadding = 16.0f;
constexpr float kDraftHeaderHeight = 58.0f;
constexpr float kDraftHeaderToBodyGap = 16.0f;
constexpr float kDraftLabelHeight = 16.0f;
constexpr float kDraftSectionGap = 8.0f;
constexpr float kDraftBodyGap = 10.0f;
constexpr float kDraftFooterHeight = 30.0f;
constexpr float kDraftMinDescriptionHeight = 54.0f;
constexpr float kDraftMinDetailHeight = 38.0f;
constexpr float kDraftLineSpacing = 3.0f;

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

float lineHeight(int fontSize)
{
    return static_cast<float>(fontSize) + kDraftLineSpacing;
}

float measureWrappedTextHeight(std::string_view text, float width, int fontSize)
{
    if (text.empty() || width <= 0.0f) {
        return lineHeight(fontSize);
    }

    int lines = 1;
    float currentLineWidth = 0.0f;
    std::string word;

    const auto flushWord = [&]() {
        if (word.empty()) {
            return;
        }

        const float wordWidth = static_cast<float>(MeasureText(word.c_str(), fontSize));
        const float spaceWidth = static_cast<float>(MeasureText(" ", fontSize));
        const float nextWidth = currentLineWidth <= 0.0f ? wordWidth : currentLineWidth + spaceWidth + wordWidth;

        if (nextWidth > width && currentLineWidth > 0.0f) {
            ++lines;
            currentLineWidth = wordWidth;
        } else {
            currentLineWidth = nextWidth;
        }

        word.clear();
    };

    for (const char c : text) {
        if (c == '\n') {
            flushWord();
            ++lines;
            currentLineWidth = 0.0f;
        } else if (c == ' ' || c == '\t') {
            flushWord();
        } else {
            word.push_back(c);
        }
    }
    flushWord();

    return static_cast<float>(lines) * lineHeight(fontSize);
}

std::string usefulWhenText(const WorldActionDraft& action)
{
    return action.usefulWhen.empty() ? "the current pressure pattern matches this strategic focus" : action.usefulWhen;
}

std::string tradeOffText(const WorldActionDraft& action)
{
    return action.tradeOff.empty() ? "coordination complexity" : action.tradeOff;
}
}

float WorldActionCardView::preferredDraftHeight(float width, const WorldActionDraft& action) const
{
    const float textWidth = std::max(0.0f, width - 2.0f * kDraftPadding);
    const float descriptionHeight = std::max(kDraftMinDescriptionHeight, measureWrappedTextHeight(action.description, textWidth, 13));
    const float usefulHeight = std::max(kDraftMinDetailHeight, measureWrappedTextHeight(usefulWhenText(action), textWidth, 12));
    const float tradeOffHeight = std::max(kDraftMinDetailHeight, measureWrappedTextHeight(tradeOffText(action), textWidth, 12));

    return kDraftPadding +
           kDraftHeaderHeight +
           kDraftHeaderToBodyGap +
           descriptionHeight +
           kDraftBodyGap +
           kDraftLabelHeight +
           kDraftSectionGap +
           usefulHeight +
           kDraftBodyGap +
           kDraftLabelHeight +
           kDraftSectionGap +
           tradeOffHeight +
           kDraftFooterHeight;
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

    const float textWidth = bounds.width - 2.0f * kDraftPadding;
    float y = bounds.y + kDraftPadding + kDraftHeaderHeight + kDraftHeaderToBodyGap;

    const float footerTop = bounds.y + bounds.height - kDraftFooterHeight;
    const float availableBodyHeight = std::max(0.0f, footerTop - y);

    const float measuredDescriptionHeight = std::max(kDraftMinDescriptionHeight, measureWrappedTextHeight(action.description, textWidth, 13));
    const float measuredUsefulHeight = std::max(kDraftMinDetailHeight, measureWrappedTextHeight(usefulWhenText(action), textWidth, 12));
    const float measuredTradeOffHeight = std::max(kDraftMinDetailHeight, measureWrappedTextHeight(tradeOffText(action), textWidth, 12));
    const float fixedBodyHeight = 2.0f * kDraftLabelHeight + 2.0f * kDraftSectionGap + 2.0f * kDraftBodyGap;
    const float measuredTextHeight = measuredDescriptionHeight + measuredUsefulHeight + measuredTradeOffHeight;
    const float textScale = measuredTextHeight > 0.0f ? std::min(1.0f, std::max(0.45f, (availableBodyHeight - fixedBodyHeight) / measuredTextHeight)) : 1.0f;

    const float descriptionHeight = measuredDescriptionHeight * textScale;
    const float usefulHeight = measuredUsefulHeight * textScale;
    const float tradeOffHeight = std::max(0.0f, footerTop - (y + descriptionHeight + kDraftBodyGap + kDraftLabelHeight + kDraftSectionGap + usefulHeight + kDraftBodyGap + kDraftLabelHeight + kDraftSectionGap));

    actions_ui::drawWrappedTextClipped(action.description, {bounds.x + kDraftPadding, y, textWidth, descriptionHeight}, 13, {205, 213, 224, 255});
    y += descriptionHeight + kDraftBodyGap;

    drawTextClipped("Useful when", {bounds.x + kDraftPadding, y, textWidth, kDraftLabelHeight}, 11, {86, 210, 151, 255});
    y += kDraftLabelHeight + kDraftSectionGap;

    actions_ui::drawWrappedTextClipped(usefulWhenText(action), {bounds.x + kDraftPadding, y, textWidth, usefulHeight}, 12, {185, 195, 210, 255});
    y += usefulHeight + kDraftBodyGap;

    drawTextClipped("May worsen", {bounds.x + kDraftPadding, y, textWidth, kDraftLabelHeight}, 11, {245, 184, 76, 255});
    y += kDraftLabelHeight + kDraftSectionGap;

    actions_ui::drawWrappedTextClipped(tradeOffText(action), {bounds.x + kDraftPadding, y, textWidth, tradeOffHeight}, 12, {185, 195, 210, 255});

    const std::string bonus = capacityBonusLabel(action.capacityBonus, true);
    drawTextClipped(bonus.empty() ? "No capacity change" : bonus, {bounds.x + kDraftPadding, bounds.y + bounds.height - 30.0f, textWidth, 18.0f}, 12, {86, 210, 151, 255});
}
