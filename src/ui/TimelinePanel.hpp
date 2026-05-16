#pragma once

#include "simulation/Simulation.hpp"
#include "ui/UiTypes.hpp"

class TimelinePanel {
public:
    void update(UiContext& context, const Simulation& simulation);
    void draw(const UiContext& context, const Simulation& simulation) const;
};
