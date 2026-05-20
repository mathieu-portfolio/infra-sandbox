#pragma once

#include "simulation/core/Simulation.hpp"
#include "ui/core/UiTypes.hpp"
#include "ui/actions/EngineeringCapacityPanel.hpp"
#include "ui/actions/EventOverlay.hpp"
#include "ui/actions/WorldActionOverlay.hpp"

class ActionPanel {
public:
    void update(UiContext& context, const Simulation& simulation);
    void draw(const UiContext& context, const Simulation& simulation) const;
    void drawPlanningOverlays(const UiContext& context, const ScenarioManager& scenarioManager) const;

private:
    EngineeringCapacityPanel engineeringCapacityPanel_{};
    EventOverlay eventOverlay_{};
    WorldActionOverlay worldActionOverlay_{};
};
