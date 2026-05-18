#pragma once

#include "ui/core/UiNode.hpp"

#include <memory>
#include <string>

namespace ui {

class GridNode : public UiNode {
public:
    explicit GridNode(int columns = 1, std::string id = {});

    Size measure(Size available) override;
    void layout(Rectangle bounds) override;

    int columns = 1;
    float columnGap = 0.0f;
    float rowGap = 0.0f;
    float fixedCellHeight = 0.0f;
};

std::unique_ptr<GridNode> grid(int columns, std::string id = {});

} // namespace ui
