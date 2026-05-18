#pragma once

#include "simulation/Node.hpp"
#include "simulation/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class OverlayController {
public:
    [[nodiscard]] Color nodeTint(const Node& node, const Simulation& simulation, const UiState& state) const;
};
