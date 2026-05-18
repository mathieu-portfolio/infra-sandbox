#pragma once

#include "ui/core/StackNode.hpp"

#include "raylib.h"

#include <string>
#include <vector>

namespace ui {

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

} // namespace ui
