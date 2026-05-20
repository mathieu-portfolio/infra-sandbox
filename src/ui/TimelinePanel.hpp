#pragma once

#include "simulation/core/Simulation.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "ui/core/UiTypes.hpp"

class TimelinePanel {
public:
    void update(UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager);
    void draw(const UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager) const;
};
