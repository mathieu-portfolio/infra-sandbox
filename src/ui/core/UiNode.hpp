#pragma once

#include "raylib.h"

#include <memory>
#include <string>
#include <vector>

namespace ui {

struct Size {
    float width = 0.0f;
    float height = 0.0f;
};

enum class Axis {
    Vertical,
    Horizontal
};

enum class SizeMode {
    Content,
    Fixed,
    Flex,
    Percent
};

enum class Align {
    Start,
    Center,
    End,
    Stretch
};

struct LayoutStyle {
    float paddingLeft = 0.0f;
    float paddingTop = 0.0f;
    float paddingRight = 0.0f;
    float paddingBottom = 0.0f;
    float gap = 0.0f;
    float minWidth = 0.0f;
    float minHeight = 0.0f;
    float maxWidth = 100000.0f;
    float maxHeight = 100000.0f;
    float fixedWidth = 0.0f;
    float fixedHeight = 0.0f;
    float percentWidth = 0.0f;
    float percentHeight = 0.0f;
    float flexGrow = 0.0f;
    SizeMode widthMode = SizeMode::Content;
    SizeMode heightMode = SizeMode::Content;
    Align crossAlign = Align::Stretch;
    bool clip = false;
    bool visible = true;
};

class UiNode {
public:
    explicit UiNode(std::string id = {});
    virtual ~UiNode() = default;

    UiNode& style(LayoutStyle style);
    UiNode& add(std::unique_ptr<UiNode> child);

    [[nodiscard]] UiNode* find(const std::string& id);
    [[nodiscard]] const UiNode* find(const std::string& id) const;

    [[nodiscard]] const std::string& id() const;
    [[nodiscard]] Rectangle bounds() const;
    [[nodiscard]] const LayoutStyle& style() const;
    [[nodiscard]] std::vector<std::unique_ptr<UiNode>>& children();
    [[nodiscard]] const std::vector<std::unique_ptr<UiNode>>& children() const;

    virtual Size measure(Size available);
    virtual void layout(Rectangle bounds);
    virtual void draw() const;

protected:
    [[nodiscard]] Size clampSize(Size size) const;
    [[nodiscard]] Rectangle contentBounds() const;

    std::string id_;
    LayoutStyle style_{};
    Rectangle bounds_{};
    Size measured_{};
    std::vector<std::unique_ptr<UiNode>> children_;
};

LayoutStyle fixedHeight(float height, float minWidth = 0.0f);
LayoutStyle flex(float grow = 1.0f, float minHeight = 0.0f);

} // namespace ui
