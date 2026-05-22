#include "app/Application.hpp"

#include "app/UiStateReset.hpp"
#include "content/ContentRegistry.hpp"

#include "ui/core/UiLayout.hpp"

#include "raylib.h"

#include <algorithm>

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;

ScenarioDefinition initializeContentAndDefaultScenario(content::ContentPackManager& packManager)
{
    (void)packManager.discoverDefaultLocations();
    (void)packManager.loadPack("vanilla");
    const content::ContentPackMetadata& metadata = content::ContentRegistry::instance().packMetadata();
    if (!metadata.defaultScenarioId.empty()) {
        const auto scenarios = ScenarioRegistry::createAll();
        const auto it = std::find_if(scenarios.begin(), scenarios.end(), [&](const ScenarioDefinition& scenario) {
            return scenario.id == metadata.defaultScenarioId;
        });
        if (it != scenarios.end()) {
            return *it;
        }
    }
    return Scenario::createDefault();
}
}

Application::Application()
    : contentPackManager_(),
      session_(initializeContentAndDefaultScenario(contentPackManager_)),
      renderer_(session_.definition())
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(kWindowWidth, kWindowHeight, "Infra Sandbox");
    SetTargetFPS(120);
    for (const auto& error : content::ContentRegistry::instance().loadErrors()) {
        TraceLog(LOG_WARNING, "Content: %s", error.c_str());
    }
    initializeLoadedScenario("Scenario loaded. Press the phase button to run the initial operational cycle.");
}

Application::~Application()
{
    renderer_.releaseResources();
    CloseWindow();
}

void Application::run()
{
    while (!WindowShouldClose()) {
        handleInput();
        cameraController_.update(GetFrameTime());
        gameplayPhaseController_.updateSimulation(GetFrameTime(), renderer_.uiManager().state(), session_, worldActionController_);

        renderer_.draw(session_.simulation(), session_.scenarioManager(), contentPackManager_, paused_, cameraController_);
    }
}

void Application::handleInput()
{
    applyPendingPackSelection();
    applyPendingScenarioSelection();
    applySandboxRequests();
    applyUiRequests();
    applyWindowRequests();

    const Vector2 dockMouse = GetMousePosition();
    auto& dockState = renderer_.uiManager().state().dockLayout;
    dockLayoutController_.update(
        dockState,
        GetScreenWidth(),
        GetScreenHeight(),
        {dockMouse.x, dockMouse.y},
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    if (dockState.activeHandle == DockResizeHandle::BottomTopEdge || dockState.hoveredHandle == DockResizeHandle::BottomTopEdge) {
        SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
    } else if (dockState.activeHandle == DockResizeHandle::LeftRightEdge || dockState.activeHandle == DockResizeHandle::RightLeftEdge
        || dockState.hoveredHandle == DockResizeHandle::LeftRightEdge || dockState.hoveredHandle == DockResizeHandle::RightLeftEdge) {
        SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    const auto events = inputManager_.poll();
    const auto simulationResult = simulationController_.handleActions(events, session_.simulation(), paused_);
    if (simulationResult.resetRequested) {
        resetScenario();
    }

    interventionController_.handleActions(events, session_.simulation(), session_.scenarioManager(), renderer_.uiManager().state(), cameraController_);
    if (!shouldBlockCameraInput()) {
        cameraController_.handleActions(events, GetFrameTime());
    }
    overlayController_.handleActions(events, renderer_.uiManager().state());
    selectionController_.handleActions(events, session_.simulation(), cameraController_, renderer_.uiManager().state());
    uiController_.handleActions(events, renderer_.uiManager().state());

    for (const auto& event : events) {
        if (event.action == InputAction::ResetSimulation && event.phase == InputPhase::Pressed) {
            gameplayPhaseController_.reset();
        }
    }

    applyPendingPackSelection();
    applyPendingScenarioSelection();
    applySandboxRequests();
    applyUiRequests();
    applyWindowRequests();
}

void Application::resetScenario()
{
    session_.reset();
    initializeLoadedScenario("Scenario reset. Press the phase button to run the initial operational cycle.");
}

void Application::loadDefaultPackScenario()
{
    const content::ContentPackMetadata& metadata = content::ContentRegistry::instance().packMetadata();
    if (!metadata.defaultScenarioId.empty() && session_.loadScenario(metadata.defaultScenarioId)) {
        initializeLoadedScenario("Content pack loaded. Press the phase button to run the default scenario's initial operational cycle.");
        return;
    }
    session_.reloadDefaultScenario();
    initializeLoadedScenario("Content pack loaded. Press the phase button to run the default scenario's initial operational cycle.");
}

void Application::loadRequestedPack(const std::string& packId)
{
    const content::ContentLoadResult result = contentPackManager_.loadPack(packId);
    if (!result.loaded) {
        for (const auto& error : result.errors) {
            TraceLog(LOG_WARNING, "Content pack: %s", error.c_str());
        }
        renderer_.uiManager().state().latestFeedback = "Unable to load content pack: " + packId;
        return;
    }
    renderer_.uiManager().releaseResources();
    loadDefaultPackScenario();
}

void Application::loadScenario(std::size_t scenarioIndex)
{
    if (!session_.loadScenario(scenarioIndex)) {
        return;
    }

    initializeLoadedScenario("Scenario loaded. Press the phase button to run the initial operational cycle.");
}

void Application::loadScenario(const std::string& scenarioId)
{
    if (!session_.loadScenario(scenarioId)) {
        return;
    }

    initializeLoadedScenario("Scenario loaded. Press the phase button to run the initial operational cycle.");
}

void Application::initializeLoadedScenario(const std::string& feedback)
{
    UiState& state = renderer_.uiManager().state();
    resetUiStateForScenario(state, feedback);
    gameplayPhaseController_.reset();
    session_.simulation().setPaused(true);
    gameplayPhaseController_.prepareScenarioGrounding(state, feedback);
}

void Application::applyPendingScenarioSelection()
{
    UiState& state = renderer_.uiManager().state();
    if (!state.requestedScenarioId.empty()) {
        const std::string scenarioId = state.requestedScenarioId;
        state.requestedScenarioId.clear();
        state.requestedScenarioIndex = -1;
        loadScenario(scenarioId);
        return;
    }

    if (state.requestedScenarioIndex < 0) {
        return;
    }

    const int scenarioIndex = state.requestedScenarioIndex;
    state.requestedScenarioIndex = -1;
    loadScenario(static_cast<std::size_t>(scenarioIndex));
}

void Application::applyPendingPackSelection()
{
    UiState& state = renderer_.uiManager().state();
    if (state.requestedPackId.empty()) {
        return;
    }

    const std::string packId = state.requestedPackId;
    state.requestedPackId.clear();
    state.requestedScenarioId.clear();
    state.requestedScenarioIndex = -1;
    loadRequestedPack(packId);
}

void Application::applySandboxRequests()
{
    UiState& state = renderer_.uiManager().state();
    const SandboxControllerResult result = sandboxController_.applyRequests(state, session_);
    if (result.resetScenarioRequested) {
        resetScenario();
    }
    if (result.resetTransitionStateRequested) {
        gameplayPhaseController_.reset();
    }
    if (result.beginTransitionRequested) {
        gameplayPhaseController_.beginTransition(state, session_, worldActionController_);
    }
}

void Application::applyUiRequests()
{
    UiState& state = renderer_.uiManager().state();
    if (state.resetScenarioRequested) {
        state.resetScenarioRequested = false;
        resetScenario();
        return;
    }
    gameplayPhaseController_.applyUiRequests(state, session_, worldActionController_);
}

void Application::applyWindowRequests()
{
    UiState& state = renderer_.uiManager().state();
    if (!state.fullscreenToggleRequested) {
        return;
    }

    if (IsWindowMaximized()) {
        RestoreWindow();
    } else {
        MaximizeWindow();
    }

    state.fullscreenToggleRequested = false;
}

bool Application::shouldBlockCameraInput() const
{
    const UiState& uiState = renderer_.uiManager().state();
    const UiLayout layout = computeUiLayout(GetScreenWidth(), GetScreenHeight(), uiState.dockLayout);
    return dockLayoutController_.isMouseOwnedByDock(uiState.dockLayout)
        || pointInUiPanel(GetMousePosition(), layout)
        || uiState.eventPopupMode != EventPopupMode::None
        || (uiState.gameplayPhase == GameplayPhase::Planning && uiState.worldActionDraftVisible && !uiState.worldActionDraft.empty());
}
