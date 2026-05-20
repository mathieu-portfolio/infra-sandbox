#pragma once

#include "simulation/topology/Node.hpp"
#include "simulation/core/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class OverlayController {
public:
    [[nodiscard]] Color nodeTint(const Node& node, const Simulation& simulation, const UiState& state) const;
};
