#pragma once

#include "simulation/core/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

class SelectionPanel {
public:
    void update(UiContext& context, const Simulation& simulation);
    void draw(const UiContext& context, const Simulation& simulation) const;
};
