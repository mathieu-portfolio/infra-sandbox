#include "app/Application.hpp"

#include "raylib.h"

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr double kFixedStepSeconds = 1.0 / 60.0;
}

Application::Application()
    : scenarioDefinition_(Scenario::createDefault()),
      simulation_(scenarioDefinition_),
      renderer_(scenarioDefinition_)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kWindowWidth, kWindowHeight, "Infra Sandbox");
    SetTargetFPS(120);
}

Application::~Application()
{
    CloseWindow();
}

void Application::run()
{
    while (!WindowShouldClose()) {
        handleInput();
        cameraController_.update(GetFrameTime());

        if (!paused_) {
            fixedStepAccumulator_ += GetFrameTime();
            while (fixedStepAccumulator_ >= kFixedStepSeconds) {
                simulation_.update(kFixedStepSeconds);
                fixedStepAccumulator_ -= kFixedStepSeconds;
            }
        }

        renderer_.draw(simulation_, paused_);
    }
}

void Application::handleInput()
{
    if (IsKeyPressed(KEY_SPACE)) {
        paused_ = !paused_;
    }

    if (IsKeyPressed(KEY_R)) {
        resetScenario();
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        simulation_.adjustClientRequestRates(1.0);
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        simulation_.adjustClientRequestRates(-1.0);
    }

    if (IsKeyPressed(KEY_ONE)) {
        simulation_.scaleProcessingCapacity(1.5);
    }

    if (IsKeyPressed(KEY_TWO)) {
        simulation_.applyCachePlaceholder();
    }

    if (IsKeyPressed(KEY_THREE)) {
        simulation_.resetProcessingCapacity();
    }
}

void Application::resetScenario()
{
    simulation_ = Simulation(scenarioDefinition_);
    fixedStepAccumulator_ = 0.0;
}
