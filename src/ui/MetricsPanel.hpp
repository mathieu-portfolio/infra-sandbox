#pragma once

#include "simulation/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

class MetricsPanel {
public:
    void update(UiContext& context, const Simulation& simulation);
    void draw(const UiContext& context, const Simulation& simulation) const;
};
