#pragma once

#include "gameplay/Scenario.hpp"
#include "simulation/Simulation.hpp"
#include "ui/UiManager.hpp"

class Renderer {
public:
    explicit Renderer(const ScenarioDefinition& scenario);

    void draw(const Simulation& simulation, bool paused);

private:
    void drawLinks(const Simulation& simulation);
    void drawNodes(const Simulation& simulation);
    void drawRequests(const Simulation& simulation);
    void drawQueueBars(const Simulation& simulation);

    ScenarioDefinition scenarioDefinition_;
    UiManager uiManager_;
};
