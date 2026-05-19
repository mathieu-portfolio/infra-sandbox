#include "app/Application.hpp"

#include "app/UiStateReset.hpp"
#include "content/ContentRegistry.hpp"

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
    initializeLoadedScenario("Scenario loaded. Running an initial operational cycle.");
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

    const auto events = inputManager_.poll();
    const auto simulationResult = simulationController_.handleActions(events, session_.simulation(), paused_);
    if (simulationResult.resetRequested) {
        resetScenario();
    }

    interventionController_.handleActions(events, session_.simulation(), session_.scenarioManager(), renderer_.uiManager().state());
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
    initializeLoadedScenario("Scenario reset. Running an initial operational cycle.");
}

void Application::loadDefaultPackScenario()
{
    const content::ContentPackMetadata& metadata = content::ContentRegistry::instance().packMetadata();
    if (!metadata.defaultScenarioId.empty() && session_.loadScenario(metadata.defaultScenarioId)) {
        initializeLoadedScenario("Content pack loaded. Running the default scenario's initial operational cycle.");
        return;
    }
    session_.reloadDefaultScenario();
    initializeLoadedScenario("Content pack loaded. Running the default scenario's initial operational cycle.");
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

    initializeLoadedScenario("Scenario loaded. Running an initial operational cycle.");
}

void Application::loadScenario(const std::string& scenarioId)
{
    if (!session_.loadScenario(scenarioId)) {
        return;
    }

    initializeLoadedScenario("Scenario loaded. Running an initial operational cycle.");
}

void Application::initializeLoadedScenario(const std::string& feedback)
{
    UiState& state = renderer_.uiManager().state();
    resetUiStateForScenario(state, feedback);
    gameplayPhaseController_.reset();
    session_.simulation().setPaused(true);
    gameplayPhaseController_.beginScenarioGroundingSimulation(state, session_, worldActionController_);
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
    gameplayPhaseController_.applyUiRequests(renderer_.uiManager().state(), session_, worldActionController_);
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
    return uiState.eventPopupMode != EventPopupMode::None
        || (uiState.gameplayPhase == GameplayPhase::Planning && uiState.worldActionDraftVisible && !uiState.worldActionDraft.empty());
}
