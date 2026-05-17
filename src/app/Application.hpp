#pragma once

#include "control/InterventionController.hpp"
#include "control/OverlayController.hpp"
#include "control/SelectionController.hpp"
#include "control/SimulationController.hpp"
#include "control/UiController.hpp"
#include "gameplay/Scenario.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "input/InputManager.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/Renderer.hpp"
#include "simulation/Simulation.hpp"

#include <cstddef>

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    void handleInput();
    void resetScenario();
    void loadScenario(std::size_t scenarioIndex);
    void applyPendingScenarioSelection();
    void applySandboxRequests();

    InputManager inputManager_;
    ScenarioDefinition scenarioDefinition_;
    ScenarioManager scenarioManager_;
    Simulation simulation_;
    Renderer renderer_;
    CameraController cameraController_;
    SimulationController simulationController_;
    InterventionController interventionController_;
    SelectionController selectionController_;
    OverlayInputController overlayController_;
    UiController uiController_;
    bool paused_ = false;
    bool stepRequested_ = false;
    double fixedStepAccumulator_ = 0.0;
};
