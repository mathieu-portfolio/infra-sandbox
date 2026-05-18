#pragma once

#include "raylib.h"

#include <functional>
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

class StackNode : public UiNode {
public:
    StackNode(Axis axis, std::string id = {});

    Size measure(Size available) override;
    void layout(Rectangle bounds) override;

private:
    Axis axis_ = Axis::Vertical;
};

class PanelNode : public UiNode {
public:
    explicit PanelNode(std::string id = {});
    void draw() const override;

    Color background{0, 0, 0, 0};
    Color border{0, 0, 0, 0};
    float radius = 0.0f;
};

class TextBlockNode : public UiNode {
public:
    TextBlockNode(std::string text, int fontSize, std::string id = {});

    Size measure(Size available) override;
    void draw() const override;

    std::string text;
    int fontSize = 12;
    Color color{230, 237, 243, 255};
    int maxLines = 4;
};

class ButtonNode : public TextBlockNode {
public:
    ButtonNode(std::string text, int fontSize, std::string id = {});
    void draw() const override;

    Color background{97, 64, 196, 180};
    Color border{145, 109, 255, 160};
    float radius = 0.08f;
};

class ScrollAreaNode : public StackNode {
public:
    explicit ScrollAreaNode(std::string id = {});
    void draw() const override;
};

class CardListNode : public UiNode {
public:
    explicit CardListNode(std::string id = {});

    Size measure(Size available) override;
    void layout(Rectangle bounds) override;

    std::vector<float> cardHeights;
    int columns = 1;
};

std::unique_ptr<StackNode> verticalStack(std::string id = {});
std::unique_ptr<StackNode> horizontalStack(std::string id = {});
LayoutStyle fixedHeight(float height, float minWidth = 0.0f);
LayoutStyle flex(float grow = 1.0f, float minHeight = 0.0f);

} // namespace ui
