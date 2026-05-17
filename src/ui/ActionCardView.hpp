#pragma once

#include "ui/ActionPanelModel.hpp"

class ActionCardView {
public:
    [[nodiscard]] float measureHeight(const ActionCard& card, float width) const;
    void draw(const ActionCard& card, bool highlighted) const;
};
