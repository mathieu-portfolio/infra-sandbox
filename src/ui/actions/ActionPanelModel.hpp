#pragma once

#include "simulation/Simulation.hpp"
#include "ui/core/UiTypes.hpp"
#include "ui/actions/cards/ActionCardModel.hpp"

#include "raylib.h"

#include <vector>

class ActionPanelModel {
public:
    [[nodiscard]] std::vector<ActionCardModel> buildCards(const Simulation& simulation, const UiState& state, int screenWidth, int screenHeight) const;
    [[nodiscard]] Rectangle panelBounds(int screenWidth, int screenHeight) const;
};
