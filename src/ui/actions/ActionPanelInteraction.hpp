#pragma once

#include "simulation/core/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

class ActionPanelInteraction {
public:
    void update(UiContext& context, const Simulation& simulation) const;
};
