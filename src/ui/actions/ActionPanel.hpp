#pragma once

#include "simulation/Simulation.hpp"
#include "ui/core/UiTypes.hpp"
#include "ui/actions/EngineeringCapacityPanel.hpp"
#include "ui/actions/WorldActionOverlay.hpp"

class ActionPanel {
public:
    void update(UiContext& context, const Simulation& simulation);
    void draw(const UiContext& context, const Simulation& simulation) const;
    void drawWorldActionOverlay(const UiContext& context) const;

private:
    EngineeringCapacityPanel engineeringCapacityPanel_{};
    WorldActionOverlay worldActionOverlay_{};
};
