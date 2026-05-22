#pragma once

#include "raylib.h"

enum class InputAction {
    ResetSimulation,
    PanCamera,
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
    OverlayBottlenecks,
    OverlayRetryAmplification,
    OverlayNone,
    ScaleUp,
    ScaleOut,
    AddCache,
    AddReadReplica,
    AddQueue,
    AddRegionalCache,
    NextPlacementCandidate,
    PreviousPlacementCandidate,
    ConfirmPlacement,
    CancelPlacement,
    ToggleCache,
    ClearCache,
    ToggleRetries,
    ToggleTrafficBurst,
    ResetInterventions,
    EnableTracing,
    ThrottleTrafficUp,
    ThrottleTrafficDown,
    ToggleMetricsUI,
    ToggleGeoGrid
};

enum class InputPhase {
    Pressed,
    Held
};

struct InputEvent {
    InputAction action = InputAction::ResetSimulation;
    InputPhase phase = InputPhase::Pressed;
    Vector2 mousePosition{};
    Vector2 mouseDelta{};
};
