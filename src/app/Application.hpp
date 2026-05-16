#pragma once

#include "gameplay/Scenario.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/Renderer.hpp"
#include "simulation/Mechanics.hpp"
#include "simulation/Simulation.hpp"

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    void handleInput();
    void resetScenario();

    ScenarioDefinition scenarioDefinition_;
    Simulation simulation_;
    Renderer renderer_;
    CameraController cameraController_;
    MechanicExecutor mechanicExecutor_;
    bool paused_ = false;
    double fixedStepAccumulator_ = 0.0;
};
