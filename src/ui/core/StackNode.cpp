#include "ui/core/StackNode.hpp"

#include <algorithm>
#include <utility>

namespace ui {

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

        const bool isFlex = axis_ == Axis::Vertical
            ? child->style().heightMode == SizeMode::Flex
            : child->style().widthMode == SizeMode::Flex;

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

        const bool isFlex = axis_ == Axis::Vertical
            ? child->style().heightMode == SizeMode::Flex
            : child->style().widthMode == SizeMode::Flex;

        float main = axis_ == Axis::Vertical ? childSizes[i].height : childSizes[i].width;
        if (isFlex && flexWeight > 0.0f) {
            main = remaining * std::max(0.0f, child->style().flexGrow) / flexWeight;
        }

        const bool stretchCross = style_.crossAlign == Align::Stretch;
        if (axis_ == Axis::Vertical) {
            const float childWidth = stretchCross || child->style().widthMode == SizeMode::Flex
                ? content.width
                : std::min(content.width, childSizes[i].width);
            float childX = content.x;
            if (!stretchCross && style_.crossAlign == Align::Center) {
                childX += (content.width - childWidth) * 0.5f;
            } else if (!stretchCross && style_.crossAlign == Align::End) {
                childX += content.width - childWidth;
            }
            child->layout({childX, cursor, childWidth, main});
        } else {
            const float childHeight = stretchCross || child->style().heightMode == SizeMode::Flex
                ? content.height
                : std::min(content.height, childSizes[i].height);
            float childY = content.y;
            if (!stretchCross && style_.crossAlign == Align::Center) {
                childY += (content.height - childHeight) * 0.5f;
            } else if (!stretchCross && style_.crossAlign == Align::End) {
                childY += content.height - childHeight;
            }
            child->layout({cursor, childY, main, childHeight});
        }
        cursor += main + style_.gap;
    }
}

std::unique_ptr<StackNode> verticalStack(std::string id)
{
    return std::make_unique<StackNode>(Axis::Vertical, std::move(id));
}

std::unique_ptr<StackNode> horizontalStack(std::string id)
{
    return std::make_unique<StackNode>(Axis::Horizontal, std::move(id));
}

} // namespace ui
