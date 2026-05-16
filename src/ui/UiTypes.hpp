#pragma once

#include <array>
#include <cstddef>

enum class UiLayer {
    WorldView,
    Hud,
    Metrics,
    Selection,
    Diagnostics,
    Interventions,
    Scenario,
    Timeline,
    Explanation,
    Debug,
    Count
};

enum class OverlayMode {
    None,
    Flow,
    Latency,
    Utilization,
    Queues,
    Errors,
    Reliability,
    Complexity,
    Bottlenecks,
    RetryAmplification
};

struct UiSelection {
    int nodeId = -1;
    int requestId = -1;
    int linkId = -1;
};

struct UiState {
    OverlayMode activeOverlay = OverlayMode::None;
    UiSelection selection{};
    std::array<bool, static_cast<std::size_t>(UiLayer::Count)> enabledLayers{};
    bool showDebug = true;
    bool showMetrics = true;
    bool showHud = true;

    UiState()
    {
        enabledLayers.fill(true);
    }
};

struct UiContext {
    UiState* state = nullptr;
    int screenWidth = 0;
    int screenHeight = 0;
    bool paused = false;
};

const char* overlayModeName(OverlayMode mode);
