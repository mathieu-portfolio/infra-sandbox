#pragma once

#include "gameplay/GameplayPhaseController.hpp"
#include "gameplay/SandboxController.hpp"
#include "app/ScenarioSession.hpp"
#include "gameplay/WorldActionController.hpp"
#include "control/InterventionController.hpp"
#include "control/OverlayController.hpp"
#include "control/SelectionController.hpp"
#include "control/SimulationController.hpp"
#include "control/UiController.hpp"
#include "input/InputManager.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/Renderer.hpp"

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
    void applyUiRequests();
    void applyWindowRequests();
    bool shouldBlockCameraInput() const;

    InputManager inputManager_;
    ScenarioSession session_;
    Renderer renderer_;
    CameraController cameraController_;
    SimulationController simulationController_;
    InterventionController interventionController_;
    SelectionController selectionController_;
    OverlayInputController overlayController_;
    UiController uiController_;
    WorldActionController worldActionController_;
    GameplayPhaseController gameplayPhaseController_;
    SandboxController sandboxController_;
    bool paused_ = false;
};
