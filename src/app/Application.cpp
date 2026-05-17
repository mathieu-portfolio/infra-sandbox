#include "app/Application.hpp"

#include "content/ContentRegistry.hpp"

#include "raylib.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr double kFixedStepSeconds = 1.0 / 60.0;
}

Application::Application()
    : scenarioDefinition_(Scenario::createDefault()),
      scenarioManager_(scenarioDefinition_),
      simulation_(scenarioManager_.definition()),
      renderer_(scenarioManager_.definition())
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE);
    InitWindow(kWindowWidth, kWindowHeight, "Infra Sandbox");
    SetTargetFPS(120);
    for (const auto& error : content::ContentRegistry::instance().loadErrors()) {
        TraceLog(LOG_WARNING, "Content: %s", error.c_str());
    }
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
        simulation_.setPaused(paused_);
        cameraController_.update(GetFrameTime());

        if (!paused_) {
            fixedStepAccumulator_ += GetFrameTime() * simulation_.simulationSpeed();
            while (fixedStepAccumulator_ >= kFixedStepSeconds) {
                scenarioManager_.update(kFixedStepSeconds, simulation_);
                simulation_.update(kFixedStepSeconds);
                fixedStepAccumulator_ -= kFixedStepSeconds;
            }
        } else if (stepRequested_) {
            scenarioManager_.update(kFixedStepSeconds, simulation_);
            simulation_.update(kFixedStepSeconds);
        }
        stepRequested_ = false;

        renderer_.draw(simulation_, scenarioManager_, paused_, cameraController_);
    }
}

void Application::handleInput()
{
    applyPendingScenarioSelection();
    applySandboxRequests();
    applyUiRequests();

    const auto events = inputManager_.poll();
    const auto simulationResult = simulationController_.handleActions(events, simulation_, paused_);
    if (simulationResult.resetRequested) {
        resetScenario();
    }
    stepRequested_ = simulationResult.stepRequested;

    interventionController_.handleActions(events, simulation_, scenarioManager_, renderer_.uiManager().state());
    cameraController_.handleActions(events, GetFrameTime());
    overlayController_.handleActions(events, renderer_.uiManager().state());
    selectionController_.handleActions(events, simulation_, cameraController_, renderer_.uiManager().state());
    uiController_.handleActions(events, renderer_.uiManager().state());

    for (const auto& event : events) {
        if (event.action == InputAction::ResetSimulation && event.phase == InputPhase::Pressed) {
            fixedStepAccumulator_ = 0.0;
        }
    }

    applyPendingScenarioSelection();
    applySandboxRequests();
    applyUiRequests();
}

void Application::resetScenario()
{
    scenarioManager_.reset();
    simulation_ = Simulation(scenarioManager_.definition());
    fixedStepAccumulator_ = 0.0;
}

void Application::loadScenario(std::size_t scenarioIndex)
{
    const std::vector<ScenarioDefinition> scenarios = ScenarioRegistry::createAll();
    if (scenarioIndex >= scenarios.size()) {
        return;
    }

    scenarioDefinition_ = scenarios[scenarioIndex];
    scenarioManager_.load(scenarioDefinition_);
    simulation_ = Simulation(scenarioManager_.definition());

    UiState& state = renderer_.uiManager().state();
    state.selection = {};
    state.latestFeedback.clear();
    state.actionHistory.clear();
    state.scenarioDroplistOpen = false;
    state.objectivesDroplistOpen = false;
    fixedStepAccumulator_ = 0.0;
}

void Application::applyPendingScenarioSelection()
{
    UiState& state = renderer_.uiManager().state();
    if (state.requestedScenarioIndex < 0) {
        return;
    }

    const int scenarioIndex = state.requestedScenarioIndex;
    state.requestedScenarioIndex = -1;
    loadScenario(static_cast<std::size_t>(scenarioIndex));
}

void Application::applySandboxRequests()
{
    UiState& state = renderer_.uiManager().state();
    if (!scenarioManager_.definition().sandboxLab) {
        return;
    }
    scenarioManager_.setSandboxTrafficMultiplier(state.sandboxTrafficMultiplier);
    scenarioManager_.setSandboxLatencyMultiplier(state.sandboxLatencyMultiplier);
    scenarioManager_.setSandboxQueueBuildup(state.sandboxQueueBuildup);
    if (!state.sandboxEventRequest.empty()) {
        scenarioManager_.injectSandboxEvent(state.sandboxEventRequest);
        state.latestFeedback = "Injected lab event: " + state.sandboxEventRequest;
        state.pendingVisualFeedbackEvents.push_back({
            .kind = state.sandboxEventRequest == "recovery" ? VisualFeedbackKind::Stabilization : VisualFeedbackKind::PressureInjected,
            .label = state.sandboxEventRequest,
        });
        state.sandboxEventRequest.clear();
    }
    if (state.sandboxClearTimelineRequested) {
        state.actionHistory.clear();
        state.latestFeedback.clear();
        scenarioManager_.clearSandboxEvents();
        state.sandboxClearTimelineRequested = false;
    }
    if (state.sandboxSlowMotionRequested) {
        simulation_.setSimulationSpeed(0.25);
        paused_ = false;
        state.sandboxSlowMotionRequested = false;
    }
    if (state.sandboxStepRequested) {
        paused_ = true;
        stepRequested_ = true;
        state.sandboxStepRequested = false;
    }
    if (state.sandboxResetSimulationRequested || state.sandboxRestoreTopologyRequested) {
        resetScenario();
        state.sandboxResetSimulationRequested = false;
        state.sandboxRestoreTopologyRequested = false;
    }
    if (state.sandboxRegenerateRequested) {
        scenarioManager_.setSandboxSeed(static_cast<std::uint32_t>(state.sandboxSeed));
        simulation_ = Simulation(scenarioManager_.definition());
        fixedStepAccumulator_ = 0.0;
        state.sandboxRegenerateRequested = false;
    }
}

void Application::applyUiRequests()
{
    UiState& state = renderer_.uiManager().state();
    if (state.fullscreenToggleRequested) {
        ToggleFullscreen();
        state.fullscreenToggleRequested = false;
    }
}
