#pragma once

#include "ui/actions/cards/ActionCardModel.hpp"

class NodeActionCardView {
public:
    [[nodiscard]] float measureHeight(const ActionCardModel& card, float width) const;
    void draw(const ActionCardModel& card, bool highlighted) const;
};
