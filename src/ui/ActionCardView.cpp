#include "ui/ActionCardView.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {
constexpr float kPad = 16.0f;
constexpr float kHeaderTop = 15.0f;
constexpr float kHeaderHeight = 30.0f;
constexpr float kContentTop = 52.0f;
constexpr float kDescriptionLineHeight = 16.0f;
constexpr float kDescriptionBottomGap = 10.0f;
constexpr float kSectionTitleHeight = 14.0f;
constexpr float kSectionTitleGap = 5.0f;
constexpr float kBulletLineHeight = 16.0f;
constexpr float kSectionGap = 10.0f;
constexpr float kFooterHeight = 28.0f;
constexpr float kFooterBottomPad = 8.0f;
constexpr float kMinHeight = 198.0f;

const char* iconForAction(const ActionCard& card)
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
    return "action.generic";
}

std::string categoryLabel(const ActionCard& card)
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

std::vector<std::string> usefulPoints(const ActionCard& card)
{
    if (!card.usefulWhen.empty()) {
        return card.usefulWhen;
    }
    if (!card.positiveEffects.empty()) {
        return card.positiveEffects;
    }
    return {"local pressure matches this action"};
}

std::vector<std::string> worsenPoints(const ActionCard& card)
{
    if (!card.negativeEffects.empty()) {
        return card.negativeEffects;
    }
    if (!card.tradeOff.empty()) {
        return {card.tradeOff};
    }
    return {"adds operational complexity"};
}

int estimatedWrappedLines(const std::string& text, float width, int fontSize, int maxLines)
{
    if (text.empty() || maxLines <= 0) {
        return 0;
    }

    // Lightweight estimate only. drawTextClipped handles the actual clipping.
    // We keep this intentionally conservative so card height usually has enough room.
    const float avgCharWidth = static_cast<float>(fontSize) * 0.56f;
    const int charsPerLine = std::max(12, static_cast<int>(width / avgCharWidth));
    int lines = 1;
    int current = 0;
    for (char c : text) {
        if (c == '\n') {
            ++lines;
            current = 0;
            continue;
        }
        ++current;
        if (current >= charsPerLine && c == ' ') {
            ++lines;
            current = 0;
        }
    }
    return std::clamp(lines, 1, maxLines);
}

float descriptionHeight(const ActionCard& card, float contentWidth)
{
    const int lines = estimatedWrappedLines(card.description, contentWidth, 12, 3);
    return std::max(kDescriptionLineHeight, static_cast<float>(lines) * kDescriptionLineHeight);
}

float sectionHeight(const std::vector<std::string>& points)
{
    return kSectionTitleHeight + kSectionTitleGap + static_cast<float>(std::max<std::size_t>(1, points.size())) * kBulletLineHeight;
}

float drawPointSection(
    const char* title,
    const std::vector<std::string>& points,
    Rectangle bounds,
    Color titleColor,
    Color textColor)
{
    drawTextClipped(title, {bounds.x, bounds.y, bounds.width, kSectionTitleHeight}, 10, titleColor);

    float y = bounds.y + kSectionTitleHeight + kSectionTitleGap;
    for (const auto& point : points) {
        drawTextClipped("- " + point, {bounds.x, y, bounds.width, kBulletLineHeight}, 11, textColor);
        y += kBulletLineHeight;
    }

    return y - bounds.y;
}
}

float ActionCardView::measureHeight(const ActionCard& card, float width) const
{
    const float contentWidth = std::max(80.0f, width - kPad * 2.0f);
    const auto useful = usefulPoints(card);
    const auto worsen = worsenPoints(card);

    const float bodyHeight =
        descriptionHeight(card, contentWidth) +
        kDescriptionBottomGap +
        sectionHeight(useful) +
        kSectionGap +
        sectionHeight(worsen);

    const float total =
        kContentTop +
        bodyHeight +
        kSectionGap +
        kFooterHeight +
        kFooterBottomPad;

    return std::max(kMinHeight, total);
}

void ActionCardView::draw(const ActionCard& card, bool highlighted) const
{
    const Color border = highlighted ? Color{145, 109, 255, 230} : Color{74, 92, 120, 130};
    DrawRectangleRounded(card.bounds, 0.045f, 8, highlighted ? Color{36, 32, 58, 238} : Color{23, 30, 42, 238});
    DrawRectangleRoundedLines(card.bounds, 0.045f, 8, border);

    IconRegistry::instance().drawIcon(
        iconForAction(card),
        {card.bounds.x + kPad, card.bounds.y + kPad, 30.0f, 30.0f},
        {89, 196, 255, 255});

    drawTextClipped(
        card.name,
        {card.bounds.x + 56.0f, card.bounds.y + kHeaderTop, card.bounds.width - 72.0f, 22.0f},
        15,
        {230, 237, 243, 255});

    const float contentWidth = card.bounds.width - kPad * 2.0f;
    const float footerY = card.bounds.y + card.bounds.height - kFooterHeight - kFooterBottomPad;

    float y = card.bounds.y + kContentTop;
    const float descHeight = descriptionHeight(card, contentWidth);
    drawTextClipped(
        card.description,
        {card.bounds.x + kPad, y, contentWidth, descHeight},
        12,
        {166, 176, 192, 255});
    y += descHeight + kDescriptionBottomGap;

    const auto useful = usefulPoints(card);
    const auto worsen = worsenPoints(card);

    y += drawPointSection(
        "USEFUL WHEN",
        useful,
        {card.bounds.x + kPad, y, contentWidth, sectionHeight(useful)},
        {189, 135, 255, 255},
        {205, 213, 224, 255});
    y += kSectionGap;

    drawPointSection(
        "MAY WORSEN",
        worsen,
        {card.bounds.x + kPad, y, contentWidth - 76.0f, std::max(0.0f, footerY - y - kSectionGap)},
        {235, 86, 100, 255},
        {205, 213, 224, 255});

    DrawRectangleRounded({card.bounds.x + kPad, footerY, 86.0f, 20.0f}, 0.2f, 6, {52, 43, 91, 230});
    drawTextClipped(categoryLabel(card), {card.bounds.x + kPad + 9.0f, footerY + 4.0f, 68.0f, 14.0f}, 10, {205, 190, 255, 255});

    const std::string price = std::to_string(actionPointCost(card.engineeringCosts)) + " AP";
    drawTextClipped(price, {card.bounds.x + card.bounds.width - 64.0f, footerY + 2.0f, 52.0f, 16.0f}, 13, {86, 210, 151, 255});
}
