#include "ui/actions/cards/NodeActionCardView.hpp"

#include "ui/actions/cards/ActionCardCommon.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiPrimitives.hpp"
#include "ui/actions/ActionText.hpp"

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


float lineHeight(int fontSize)
{
    return static_cast<float>(fontSize) + 4.0f;
}

float measureWrappedTextHeight(const std::string& text, float width, int fontSize)
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

float descriptionHeight(const ActionCardModel& card, float contentWidth)
{
    return std::max(kDescriptionLineHeight, measureWrappedTextHeight(card.description, contentWidth, 12));
}

float sectionHeight(const std::vector<std::string>& points, float contentWidth)
{
    if (points.empty()) {
        return 0.0f;
    }

    float height = kSectionTitleHeight + kSectionTitleGap;
    for (const auto& point : points) {
        height += std::max(kBulletLineHeight, measureWrappedTextHeight("- " + point, contentWidth, 11));
    }
    return height;
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
        const float pointHeight = std::max(kBulletLineHeight, measureWrappedTextHeight("- " + point, bounds.width, 11));
        actions_ui::drawWrappedTextClipped("- " + point, {bounds.x, y, bounds.width, pointHeight}, 11, textColor, 4.0f);
        y += pointHeight;
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
        sectionHeight(useful, contentWidth) +
        (useful.empty() || worsen.empty() ? 0.0f : kSectionGap) +
        sectionHeight(worsen, contentWidth);

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
    actions_ui::drawWrappedTextClipped(
        card.description,
        {card.bounds.x + kPad, y, contentWidth, descHeight},
        12,
        {166, 176, 192, 255},
        4.0f);
    y += descHeight + kDescriptionBottomGap;

    const auto useful = actions_ui::cards::usefulPoints(card);
    const auto worsen = actions_ui::cards::worsenPoints(card);

    if (!useful.empty()) {
        y += drawPointSection(
            "USEFUL WHEN",
            useful,
            {card.bounds.x + kPad, y, contentWidth, sectionHeight(useful, contentWidth)},
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
            {card.bounds.x + kPad, y, contentWidth, sectionHeight(worsen, contentWidth)},
            {235, 86, 100, 255},
            {205, 213, 224, 255});
    }

    actions_ui::cards::drawChip(
        {card.bounds.x + kPad, footerY, 86.0f, 20.0f},
        actions_ui::cards::categoryLabel(card),
        {52, 43, 91, 230},
        {205, 190, 255, 255});

    std::string price = engineeringCostLabel(card.engineeringCosts);
    if (card.useLimit > 0) {
        price += " | " + std::to_string(card.usesRemaining) + "/" + std::to_string(card.useLimit);
    }
    drawTextClipped(price, {card.bounds.x + card.bounds.width - 162.0f, footerY + 2.0f, 150.0f, 16.0f}, 12, {86, 210, 151, 255});
}
