#include "ui/core/UiNode.hpp"

#include <algorithm>
#include <utility>

namespace ui {

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
Size UiNode::measuredSize() const { return measured_; }
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
        if (!child->style().visible) {
            continue;
        }
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
        if (child->style().visible) {
            child->layout(content);
        }
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

LayoutStyle contentSize(float minWidth, float minHeight)
{
    LayoutStyle style;
    style.widthMode = SizeMode::Content;
    style.heightMode = SizeMode::Content;
    style.minWidth = minWidth;
    style.minHeight = minHeight;
    return style;
}

LayoutStyle fixedSize(float width, float height)
{
    LayoutStyle style;
    style.widthMode = SizeMode::Fixed;
    style.heightMode = SizeMode::Fixed;
    style.fixedWidth = width;
    style.fixedHeight = height;
    return style;
}

LayoutStyle fixedWidth(float width, float minHeight)
{
    LayoutStyle style;
    style.widthMode = SizeMode::Fixed;
    style.fixedWidth = width;
    style.minHeight = minHeight;
    return style;
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
