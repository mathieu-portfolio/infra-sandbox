#include "ui/core/BasicNodes.hpp"

#include "ui/core/UiPrimitives.hpp"

#include <algorithm>
#include <utility>

namespace ui {
namespace {
} // namespace

PanelNode::PanelNode(std::string id) : UiNode(std::move(id)) {}

void PanelNode::draw() const
{
    if (background.a > 0) {
        DrawRectangleRounded(bounds_, radius, 8, background);
    }
    if (border.a > 0) {
        DrawRectangleRoundedLines(bounds_, radius, 8, border);
    }
    UiNode::draw();
}

TextBlockNode::TextBlockNode(std::string textValue, int fontSizeValue, std::string id)
    : UiNode(std::move(id)), text(std::move(textValue)), fontSize(fontSizeValue)
{
}

Size TextBlockNode::measure(Size available)
{
    const float horizontalPadding = style_.paddingLeft + style_.paddingRight;
    const float verticalPadding = style_.paddingTop + style_.paddingBottom;
    const float availableTextWidth = std::max(1.0f, available.width - horizontalPadding);
    const float naturalTextWidth = static_cast<float>(std::max(MeasureText(text.c_str(), fontSize), 1));
    const float textWidth = style_.widthMode == SizeMode::Fixed
        ? std::max(1.0f, style_.fixedWidth - horizontalPadding)
        : std::min(availableTextWidth, naturalTextWidth);
    const float textHeight = measureTextWrappedHeight(text, textWidth, fontSize, maxLines, 0.0f);
    measured_ = clampSize({textWidth + horizontalPadding, textHeight + verticalPadding});
    return measured_;
}

void TextBlockNode::draw() const
{
    drawTextWrappedClipped(text, contentBounds(), fontSize, color, 0.0f);
    UiNode::draw();
}

ButtonNode::ButtonNode(std::string textValue, int fontSizeValue, std::string id)
    : TextBlockNode(std::move(textValue), fontSizeValue, std::move(id))
{
    style_.paddingLeft = 14.0f;
    style_.paddingTop = 9.0f;
    style_.paddingRight = 14.0f;
    style_.paddingBottom = 9.0f;
}

void ButtonNode::draw() const
{
    DrawRectangleRounded(bounds_, radius, 8, background);
    DrawRectangleRoundedLines(bounds_, radius, 8, border);

    const int textWidth = MeasureText(text.c_str(), fontSize);
    const float textX = bounds_.x + (bounds_.width - static_cast<float>(textWidth)) * 0.5f;
    const float textY = bounds_.y + (bounds_.height - static_cast<float>(fontSize)) * 0.5f;
    DrawText(text.c_str(), static_cast<int>(textX), static_cast<int>(textY), fontSize, color);
    UiNode::draw();
}

ScrollAreaNode::ScrollAreaNode(std::string id) : StackNode(Axis::Vertical, std::move(id))
{
    style_.clip = true;
}

void ScrollAreaNode::draw() const
{
    BeginScissorMode(static_cast<int>(bounds_.x), static_cast<int>(bounds_.y), static_cast<int>(bounds_.width), static_cast<int>(bounds_.height));
    StackNode::draw();
    EndScissorMode();
}

CardListNode::CardListNode(std::string id) : UiNode(std::move(id)) {}

Size CardListNode::measure(Size available)
{
    const int safeColumns = std::max(1, columns);
    float height = 0.0f;
    float rowHeight = 0.0f;

    for (std::size_t i = 0; i < cardHeights.size(); ++i) {
        rowHeight = std::max(rowHeight, cardHeights[i]);
        if ((static_cast<int>(i) + 1) % safeColumns == 0) {
            height += rowHeight + style_.gap;
            rowHeight = 0.0f;
        }
    }
    if (rowHeight > 0.0f) {
        height += rowHeight;
    }

    measured_ = clampSize({available.width, height});
    return measured_;
}

void CardListNode::layout(Rectangle bounds)
{
    bounds_ = bounds;
}

} // namespace ui
