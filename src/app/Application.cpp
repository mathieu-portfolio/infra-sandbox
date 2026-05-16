#include "app/Application.hpp"

#include "raylib.h"

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr double kFixedStepSeconds = 1.0 / 60.0;
}

Application::Application()
    : scenarioDefinition_(Scenario::createDefault()),
      scenarioManager_(scenarioDefinition_),
      simulation_(scenarioDefinition_),
      renderer_(scenarioDefinition_)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kWindowWidth, kWindowHeight, "Infra Sandbox");
    SetTargetFPS(120);
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
    const auto events = inputManager_.poll();
    const auto simulationResult = simulationController_.handleActions(events, simulation_, paused_);
    if (simulationResult.resetRequested) {
        resetScenario();
    }
    stepRequested_ = simulationResult.stepRequested;

    interventionController_.handleActions(events, simulation_);
    cameraController_.handleActions(events, GetFrameTime());
    overlayController_.handleActions(events, renderer_.uiManager().state());
    selectionController_.handleActions(events, simulation_, cameraController_, renderer_.uiManager().state());
    uiController_.handleActions(events, renderer_.uiManager().state());

    for (const auto& event : events) {
        if (event.action == InputAction::ResetSimulation && event.phase == InputPhase::Pressed) {
            fixedStepAccumulator_ = 0.0;
        }
    }
}

void Application::resetScenario()
{
    scenarioManager_.reset();
    simulation_ = Simulation(scenarioManager_.definition());
    fixedStepAccumulator_ = 0.0;
}
