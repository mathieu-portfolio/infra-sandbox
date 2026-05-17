#pragma once

#include "simulation/Simulation.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "ui/UiTypes.hpp"

class HudPanel {
public:
    void update(UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager);
    void draw(const UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager) const;
};
