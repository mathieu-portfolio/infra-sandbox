#include "ui/actions/cards/NodeActionCardView.hpp"

#include "ui/actions/cards/ActionCardCommon.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiPrimitives.hpp"

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

float descriptionHeight(const ActionCardModel& card, float contentWidth)
{
    const int lines = estimatedWrappedLines(card.description, contentWidth, 12, 3);
    return std::max(kDescriptionLineHeight, static_cast<float>(lines) * kDescriptionLineHeight);
}

float sectionHeight(const std::vector<std::string>& points)
{
    if (points.empty()) {
        return 0.0f;
    }
    return kSectionTitleHeight + kSectionTitleGap + static_cast<float>(points.size()) * kBulletLineHeight;
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

const char* domainShort(EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend: return "FE";
    case EngineeringDomain::Backend: return "BE";
    case EngineeringDomain::Infrastructure: return "INF";
    case EngineeringDomain::Data: return "DATA";
    case EngineeringDomain::Operations: return "OPS";
    case EngineeringDomain::Count: break;
    }
    return "";
}

std::string engineeringCostLabel(const std::vector<EngineeringCost>& costs)
{
    if (costs.empty()) {
        return "No AP";
    }
    std::string label;
    for (const auto& cost : costs) {
        if (cost.amount <= 0) {
            continue;
        }
        if (!label.empty()) {
            label += "  ";
        }
        label += domainShort(cost.domain);
        label += " ";
        label += std::to_string(cost.amount);
    }
    return label.empty() ? "No AP" : label;
}

float NodeActionCardView::measureHeight(const ActionCardModel& card, float width) const
{
    const float contentWidth = std::max(80.0f, width - kPad * 2.0f);
    const auto useful = actions_ui::cards::usefulPoints(card);
    const auto worsen = actions_ui::cards::worsenPoints(card);

    const float bodyHeight =
        descriptionHeight(card, contentWidth) +
        kDescriptionBottomGap +
        sectionHeight(useful) +
        (useful.empty() || worsen.empty() ? 0.0f : kSectionGap) +
        sectionHeight(worsen);

    const float total =
        kContentTop +
        bodyHeight +
        kSectionGap +
        kFooterHeight +
        kFooterBottomPad;

    return std::max(kMinHeight, total);
}

void NodeActionCardView::draw(const ActionCardModel& card, bool highlighted) const
{
    actions_ui::cards::drawCardChrome(card.bounds, {.highlighted = highlighted});

    IconRegistry::instance().drawIcon(
        actions_ui::cards::iconForActionCard(card),
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

    const auto useful = actions_ui::cards::usefulPoints(card);
    const auto worsen = actions_ui::cards::worsenPoints(card);

    if (!useful.empty()) {
        y += drawPointSection(
            "USEFUL WHEN",
            useful,
            {card.bounds.x + kPad, y, contentWidth, sectionHeight(useful)},
            {189, 135, 255, 255},
            {205, 213, 224, 255});
        if (!worsen.empty()) {
            y += kSectionGap;
        }
    }

    if (!worsen.empty()) {
        drawPointSection(
            "MAY WORSEN",
            worsen,
            {card.bounds.x + kPad, y, contentWidth - 76.0f, std::max(0.0f, footerY - y - kSectionGap)},
            {235, 86, 100, 255},
            {205, 213, 224, 255});
    }

    actions_ui::cards::drawChip(
        {card.bounds.x + kPad, footerY, 86.0f, 20.0f},
        actions_ui::cards::categoryLabel(card),
        {52, 43, 91, 230},
        {205, 190, 255, 255});

    const std::string price = engineeringCostLabel(card.engineeringCosts);
    drawTextClipped(price, {card.bounds.x + card.bounds.width - 142.0f, footerY + 2.0f, 130.0f, 16.0f}, 12, {86, 210, 151, 255});
}
