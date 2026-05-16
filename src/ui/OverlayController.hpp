#pragma once

#include "simulation/Node.hpp"
#include "simulation/Simulation.hpp"
#include "ui/UiTypes.hpp"

#include "raylib.h"

class OverlayController {
public:
    void update(UiState& state);

    [[nodiscard]] Color nodeTint(const Node& node, const Simulation& simulation, const UiState& state) const;
};
