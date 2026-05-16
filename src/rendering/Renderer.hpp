#pragma once

#include "gameplay/Scenario.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "rendering/CameraController.hpp"
#include "simulation/Simulation.hpp"
#include "ui/UiManager.hpp"

class Renderer {
public:
    explicit Renderer(const ScenarioDefinition& scenario);

    void draw(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused, const CameraController& camera);

private:
    void drawLinks(const Simulation& simulation, const CameraController& camera);
    void drawNodes(const Simulation& simulation, const CameraController& camera);
    void drawRequests(const Simulation& simulation, const CameraController& camera);
    void drawQueueBars(const Simulation& simulation, const CameraController& camera);

public:
    [[nodiscard]] UiManager& uiManager();
    [[nodiscard]] const UiManager& uiManager() const;

private:
    ScenarioDefinition scenarioDefinition_;
    UiManager uiManager_;
};
