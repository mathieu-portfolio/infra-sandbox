#pragma once

#include "simulation/core/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

class MetricsPanelRenderer {
public:
    void draw(const UiContext& context, const Simulation& simulation) const;
};
