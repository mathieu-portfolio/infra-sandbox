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
            fixedStepAccumulator_ += GetFrameTime() * simulation_.simulationSpeed();
            while (fixedStepAccumulator_ >= kFixedStepSeconds) {
                simulation_.update(kFixedStepSeconds);
                fixedStepAccumulator_ -= kFixedStepSeconds;
            }
        } else if (IsKeyPressed(KEY_PERIOD)) {
            simulation_.update(kFixedStepSeconds);
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
        mechanicExecutor_.execute(simulation_, {MechanicType::ThrottleTraffic, -1, 1.0});
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        mechanicExecutor_.execute(simulation_, {MechanicType::ThrottleTraffic, -1, -1.0});
    }

    if (IsKeyPressed(KEY_ONE)) {
        simulation_.setSimulationSpeed(1.0);
    }

    if (IsKeyPressed(KEY_TWO)) {
        mechanicExecutor_.execute(simulation_, {MechanicType::EnableCache});
    }

    if (IsKeyPressed(KEY_THREE)) {
        simulation_.setSimulationSpeed(2.0);
    }

    if (IsKeyPressed(KEY_FIVE)) {
        simulation_.setSimulationSpeed(5.0);
    }

    if (IsKeyPressed(KEY_A)) {
        mechanicExecutor_.execute(simulation_, {MechanicType::ScaleUp, -1, 1.5});
    }

    if (IsKeyPressed(KEY_FOUR)) {
        simulation_.resetProcessingCapacity();
    }

    if (IsKeyPressed(KEY_C)) {
        mechanicExecutor_.execute(simulation_, {MechanicType::ClearCache});
    }

    if (IsKeyPressed(KEY_B)) {
        simulation_.toggleBurstMode();
    }

    if (IsKeyPressed(KEY_T)) {
        mechanicExecutor_.execute(simulation_, {MechanicType::ToggleRetries});
    }
}

void Application::resetScenario()
{
    simulation_ = Simulation(scenarioDefinition_);
    fixedStepAccumulator_ = 0.0;
}
