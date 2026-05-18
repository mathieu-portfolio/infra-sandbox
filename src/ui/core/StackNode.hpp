#pragma once

#include "ui/core/UiNode.hpp"

#include <memory>
#include <string>

namespace ui {

class StackNode : public UiNode {
public:
    StackNode(Axis axis, std::string id = {});

    Size measure(Size available) override;
    void layout(Rectangle bounds) override;

private:
    Axis axis_ = Axis::Vertical;
};

std::unique_ptr<StackNode> verticalStack(std::string id = {});
std::unique_ptr<StackNode> horizontalStack(std::string id = {});

} // namespace ui
