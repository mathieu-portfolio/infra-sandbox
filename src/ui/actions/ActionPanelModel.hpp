#pragma once

#include "simulation/core/Simulation.hpp"
#include "ui/core/UiTypes.hpp"
#include "ui/actions/cards/ActionCardModel.hpp"

#include "raylib.h"

#include <vector>

struct ActionSectionsLayout {
    Rectangle title{};
    Rectangle capacity{};
    Rectangle filters[4]{};
    Rectangle actionList{};
};

class ActionPanelModel {
public:
    [[nodiscard]] std::vector<ActionCardModel> buildCards(const Simulation& simulation, const UiState& state, int screenWidth, int screenHeight) const;
    [[nodiscard]] Rectangle panelBounds(int screenWidth, int screenHeight) const;
    [[nodiscard]] ActionSectionsLayout actionSectionsLayout(const UiState& state, int screenWidth, int screenHeight) const;
};
