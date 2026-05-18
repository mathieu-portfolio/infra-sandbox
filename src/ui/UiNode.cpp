#include "ui/UiNode.hpp"

#include "ui/UiPrimitives.hpp"

#include <algorithm>
#include <numeric>

namespace ui {
namespace {
float textHeightForWidth(const std::string& text, float width, int fontSize, int maxLines)
{
    if (text.empty()) {
        return static_cast<float>(fontSize);
    }
    const float avgCharWidth = static_cast<float>(fontSize) * 0.56f;
    const int charsPerLine = std::max(8, static_cast<int>(width / std::max(1.0f, avgCharWidth)));
    int lines = 1;
    int current = 0;
    for (char c : text) {
        if (c == '\n') {
            ++lines;
            current = 0;
        } else {
            ++current;
            if (current >= charsPerLine && c == ' ') {
                ++lines;
                current = 0;
            }
        }
    }
    return static_cast<float>(std::clamp(lines, 1, std::max(1, maxLines)) * fontSize);
}
}

UiNode::UiNode(std::string id) : id_(std::move(id)) {}

UiNode& UiNode::style(LayoutStyle style)
{
    style_ = style;
    return *this;
}

UiNode& UiNode::add(std::unique_ptr<UiNode> child)
{
    children_.push_back(std::move(child));
    return *this;
}

UiNode* UiNode::find(const std::string& id)
{
    if (id_ == id) {
        return this;
    }
    for (auto& child : children_) {
        if (UiNode* found = child->find(id); found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

const UiNode* UiNode::find(const std::string& id) const
{
    if (id_ == id) {
        return this;
    }
    for (const auto& child : children_) {
        if (const UiNode* found = child->find(id); found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

const std::string& UiNode::id() const { return id_; }
Rectangle UiNode::bounds() const { return bounds_; }
const LayoutStyle& UiNode::style() const { return style_; }
std::vector<std::unique_ptr<UiNode>>& UiNode::children() { return children_; }
const std::vector<std::unique_ptr<UiNode>>& UiNode::children() const { return children_; }

Size UiNode::measure(Size available)
{
    Size measured{};
    const Size childAvailable{
        std::max(0.0f, available.width - style_.paddingLeft - style_.paddingRight),
        std::max(0.0f, available.height - style_.paddingTop - style_.paddingBottom),
    };
    for (auto& child : children_) {
        const Size childSize = child->measure(childAvailable);
        measured.width = std::max(measured.width, childSize.width);
        measured.height = std::max(measured.height, childSize.height);
    }
    measured.width += style_.paddingLeft + style_.paddingRight;
    measured.height += style_.paddingTop + style_.paddingBottom;
    measured_ = clampSize(measured);
    return measured_;
}

void UiNode::layout(Rectangle bounds)
{
    bounds_ = bounds;
    const Rectangle content = contentBounds();
    for (auto& child : children_) {
        child->layout(content);
    }
}

void UiNode::draw() const
{
    if (!style_.visible) {
        return;
    }
    if (style_.clip) {
        BeginScissorMode(static_cast<int>(bounds_.x), static_cast<int>(bounds_.y), static_cast<int>(bounds_.width), static_cast<int>(bounds_.height));
    }
    for (const auto& child : children_) {
        child->draw();
    }
    if (style_.clip) {
        EndScissorMode();
    }
}

Size UiNode::clampSize(Size size) const
{
    if (style_.widthMode == SizeMode::Fixed) {
        size.width = style_.fixedWidth;
    }
    if (style_.heightMode == SizeMode::Fixed) {
        size.height = style_.fixedHeight;
    }
    return {
        std::clamp(size.width, style_.minWidth, style_.maxWidth),
        std::clamp(size.height, style_.minHeight, style_.maxHeight),
    };
}

Rectangle UiNode::contentBounds() const
{
    return {
        bounds_.x + style_.paddingLeft,
        bounds_.y + style_.paddingTop,
        std::max(0.0f, bounds_.width - style_.paddingLeft - style_.paddingRight),
        std::max(0.0f, bounds_.height - style_.paddingTop - style_.paddingBottom),
    };
}

StackNode::StackNode(Axis axis, std::string id) : UiNode(std::move(id)), axis_(axis) {}

Size StackNode::measure(Size available)
{
    Size measured{};
    const Size childAvailable{
        std::max(0.0f, available.width - style_.paddingLeft - style_.paddingRight),
        std::max(0.0f, available.height - style_.paddingTop - style_.paddingBottom),
    };
    int visibleChildren = 0;
    for (auto& child : children_) {
        if (!child->style().visible) {
            continue;
        }
        const Size childSize = child->measure(childAvailable);
        ++visibleChildren;
        if (axis_ == Axis::Vertical) {
            measured.width = std::max(measured.width, childSize.width);
            measured.height += childSize.height;
        } else {
            measured.width += childSize.width;
            measured.height = std::max(measured.height, childSize.height);
        }
    }
    const float gaps = visibleChildren > 1 ? static_cast<float>(visibleChildren - 1) * style_.gap : 0.0f;
    if (axis_ == Axis::Vertical) {
        measured.height += gaps;
    } else {
        measured.width += gaps;
    }
    measured.width += style_.paddingLeft + style_.paddingRight;
    measured.height += style_.paddingTop + style_.paddingBottom;
    measured_ = clampSize(measured);
    return measured_;
}

void StackNode::layout(Rectangle bounds)
{
    bounds_ = bounds;
    const Rectangle content = contentBounds();
    float fixedMain = 0.0f;
    float flexWeight = 0.0f;
    int visibleChildren = 0;
    std::vector<Size> childSizes(children_.size());
    for (std::size_t i = 0; i < children_.size(); ++i) {
        auto& child = children_[i];
        if (!child->style().visible) {
            continue;
        }
        ++visibleChildren;
        const Size measured = child->measure({content.width, content.height});
        childSizes[i] = measured;
        const bool isFlex = axis_ == Axis::Vertical ? child->style().heightMode == SizeMode::Flex : child->style().widthMode == SizeMode::Flex;
        if (isFlex) {
            flexWeight += std::max(0.0f, child->style().flexGrow);
        } else {
            fixedMain += axis_ == Axis::Vertical ? measured.height : measured.width;
        }
    }
    const float gaps = visibleChildren > 1 ? static_cast<float>(visibleChildren - 1) * style_.gap : 0.0f;
    const float availableMain = axis_ == Axis::Vertical ? content.height : content.width;
    const float remaining = std::max(0.0f, availableMain - fixedMain - gaps);
    float cursor = axis_ == Axis::Vertical ? content.y : content.x;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        auto& child = children_[i];
        if (!child->style().visible) {
            continue;
        }
        const bool isFlex = axis_ == Axis::Vertical ? child->style().heightMode == SizeMode::Flex : child->style().widthMode == SizeMode::Flex;
        float main = axis_ == Axis::Vertical ? childSizes[i].height : childSizes[i].width;
        if (isFlex && flexWeight > 0.0f) {
            main = remaining * std::max(0.0f, child->style().flexGrow) / flexWeight;
        }
        if (axis_ == Axis::Vertical) {
            child->layout({content.x, cursor, content.width, main});
        } else {
            child->layout({cursor, content.y, main, content.height});
        }
        cursor += main + style_.gap;
    }
}

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
    const float width = style_.widthMode == SizeMode::Fixed ? style_.fixedWidth : std::min(available.width, static_cast<float>(std::max(MeasureText(text.c_str(), fontSize), 1)));
    const float height = textHeightForWidth(text, std::max(1.0f, available.width), fontSize, maxLines);
    measured_ = clampSize({width, height});
    return measured_;
}

void TextBlockNode::draw() const
{
    drawTextClipped(text, bounds_, fontSize, color);
    UiNode::draw();
}

ButtonNode::ButtonNode(std::string textValue, int fontSizeValue, std::string id)
    : TextBlockNode(std::move(textValue), fontSizeValue, std::move(id))
{
}

void ButtonNode::draw() const
{
    DrawRectangleRounded(bounds_, radius, 8, background);
    DrawRectangleRoundedLines(bounds_, radius, 8, border);
    TextBlockNode::draw();
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

std::unique_ptr<StackNode> verticalStack(std::string id)
{
    return std::make_unique<StackNode>(Axis::Vertical, std::move(id));
}

std::unique_ptr<StackNode> horizontalStack(std::string id)
{
    return std::make_unique<StackNode>(Axis::Horizontal, std::move(id));
}

LayoutStyle fixedHeight(float height, float minWidth)
{
    LayoutStyle style;
    style.heightMode = SizeMode::Fixed;
    style.fixedHeight = height;
    style.minWidth = minWidth;
    return style;
}

LayoutStyle flex(float grow, float minHeight)
{
    LayoutStyle style;
    style.heightMode = SizeMode::Flex;
    style.flexGrow = grow;
    style.minHeight = minHeight;
    return style;
}

} // namespace ui
