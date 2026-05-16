#pragma once

#include "raylib.h"

enum class InputAction {
    PauseSimulation,
    StepSimulation,
    ResetSimulation,
    IncreaseSimulationSpeed,
    DecreaseSimulationSpeed,
    SetSimulationSpeed1x,
    SetSimulationSpeed2x,
    SetSimulationSpeed5x,
    MoveCameraUp,
    MoveCameraDown,
    MoveCameraLeft,
    MoveCameraRight,
    ZoomIn,
    ZoomOut,
    ResetCamera,
    Select,
    MultiSelect,
    ClearSelection,
    OverlayFlow,
    OverlayLatency,
    OverlayUtilization,
    OverlayQueues,
    OverlayErrors,
    OverlayReliability,
    OverlayComplexity,
    OverlayNone,
    ScaleUp,
    ScaleOut,
    ToggleCache,
    ClearCache,
    ToggleRetries,
    ToggleTrafficBurst,
    ResetInterventions,
    EnableTracing,
    ThrottleTrafficUp,
    ThrottleTrafficDown,
    ToggleDebugUI,
    ToggleMetricsUI
};

enum class InputPhase {
    Pressed,
    Held
};

struct InputEvent {
    InputAction action = InputAction::PauseSimulation;
    InputPhase phase = InputPhase::Pressed;
    Vector2 mousePosition{};
};
