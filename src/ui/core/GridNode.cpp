#include "ui/core/GridNode.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace ui {

GridNode::GridNode(int columnCount, std::string id) : UiNode(std::move(id)), columns(std::max(1, columnCount)) {}

Size GridNode::measure(Size available)
{
    const int safeColumns = std::max(1, columns);
    const float gapX = columnGap > 0.0f ? columnGap : style_.gap;
    const float gapY = rowGap > 0.0f ? rowGap : style_.gap;
    const Size childAvailable{
        std::max(0.0f, available.width - style_.paddingLeft - style_.paddingRight),
        std::max(0.0f, available.height - style_.paddingTop - style_.paddingBottom),
    };
    const float totalColumnGap = static_cast<float>(safeColumns - 1) * gapX;
    const float cellWidth = std::max(0.0f, (childAvailable.width - totalColumnGap) / static_cast<float>(safeColumns));

    std::vector<float> rowHeights;
    int visibleIndex = 0;
    for (auto& child : children_) {
        if (!child->style().visible) {
            continue;
        }
        const int row = visibleIndex / safeColumns;
        if (row >= static_cast<int>(rowHeights.size())) {
            rowHeights.push_back(0.0f);
        }
        const Size childSize = child->measure({cellWidth, childAvailable.height});
        rowHeights[static_cast<std::size_t>(row)] = std::max(
            rowHeights[static_cast<std::size_t>(row)],
            fixedCellHeight > 0.0f ? fixedCellHeight : childSize.height);
        ++visibleIndex;
    }

    float height = 0.0f;
    for (float rowHeight : rowHeights) {
        height += rowHeight;
    }
    if (rowHeights.size() > 1) {
        height += static_cast<float>(rowHeights.size() - 1) * gapY;
    }

    measured_ = clampSize({
        childAvailable.width + style_.paddingLeft + style_.paddingRight,
        height + style_.paddingTop + style_.paddingBottom,
    });
    return measured_;
}

void GridNode::layout(Rectangle bounds)
{
    bounds_ = bounds;
    const int safeColumns = std::max(1, columns);
    const float gapX = columnGap > 0.0f ? columnGap : style_.gap;
    const float gapY = rowGap > 0.0f ? rowGap : style_.gap;
    const Rectangle content = contentBounds();
    const float totalColumnGap = static_cast<float>(safeColumns - 1) * gapX;
    const float cellWidth = std::max(0.0f, (content.width - totalColumnGap) / static_cast<float>(safeColumns));

    std::vector<float> rowHeights;
    int visibleIndex = 0;
    for (auto& child : children_) {
        if (!child->style().visible) {
            continue;
        }
        const int row = visibleIndex / safeColumns;
        if (row >= static_cast<int>(rowHeights.size())) {
            rowHeights.push_back(0.0f);
        }
        const Size childSize = child->measure({cellWidth, content.height});
        rowHeights[static_cast<std::size_t>(row)] = std::max(
            rowHeights[static_cast<std::size_t>(row)],
            fixedCellHeight > 0.0f ? fixedCellHeight : childSize.height);
        ++visibleIndex;
    }

    std::vector<float> rowY(rowHeights.size(), content.y);
    float cursorY = content.y;
    for (std::size_t row = 0; row < rowHeights.size(); ++row) {
        rowY[row] = cursorY;
        cursorY += rowHeights[row] + gapY;
    }

    visibleIndex = 0;
    for (auto& child : children_) {
        if (!child->style().visible) {
            continue;
        }
        const int row = visibleIndex / safeColumns;
        const int column = visibleIndex % safeColumns;
        const float x = content.x + static_cast<float>(column) * (cellWidth + gapX);
        const float y = rowY[static_cast<std::size_t>(row)];
        const float height = rowHeights[static_cast<std::size_t>(row)];
        child->layout({x, y, cellWidth, height});
        ++visibleIndex;
    }
}

std::unique_ptr<GridNode> grid(int columns, std::string id)
{
    return std::make_unique<GridNode>(columns, std::move(id));
}

} // namespace ui
