#pragma once

#include "gameplay/GameplayPhaseController.hpp"
#include "gameplay/SandboxController.hpp"
#include "app/ScenarioSession.hpp"
#include "content/loading/ContentPackManager.hpp"
#include "gameplay/WorldActionController.hpp"
#include "control/InterventionController.hpp"
#include "control/OverlayController.hpp"
#include "control/SelectionController.hpp"
#include "control/SimulationController.hpp"
#include "control/UiController.hpp"
#include "input/InputManager.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/Renderer.hpp"
#include "ui/core/DockLayout.hpp"

#include <cstddef>
#include <string>

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    void handleInput();
    void resetScenario();
    void loadScenario(std::size_t scenarioIndex);
    void loadScenario(const std::string& scenarioId);
    void loadDefaultPackScenario();
    void loadRequestedPack(const std::string& packId);
    void initializeLoadedScenario(const std::string& feedback);
    void applyPendingScenarioSelection();
    void applyPendingPackSelection();
    void applySandboxRequests();
    void applyUiRequests();
    void applyWindowRequests();
    bool shouldBlockCameraInput() const;

    InputManager inputManager_;
    content::ContentPackManager contentPackManager_;
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
    DockLayoutController dockLayoutController_;
    bool paused_ = false;
};
