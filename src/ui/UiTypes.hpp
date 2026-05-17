#pragma once

#include "simulation/Metrics.hpp"
#include "simulation/TopologyMutation.hpp"

#include <array>
#include <cstddef>
#include <deque>
#include <string>

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
    Flow,
    Resources,
    Persistence,
    Reliability,
    Geography,
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

enum class TimelineCategory {
    All,
    Traffic,
    Change,
    Database,
    System,
    Objectives,
    Reliability
};

enum class TimelineFilter {
    RecentFirst,
    OldestFirst,
    ActiveOnly
};

struct UiSelection {
    int nodeId = -1;
    int requestId = -1;
    int linkId = -1;
};

struct ActionFeedback {
    double timeSeconds = 0.0;
    std::string actionName;
    std::string target;
    std::string message;
    MetricsSnapshot beforeMetrics{};
    bool observationPending = false;
    bool observationRecorded = false;
    double observeAfterSeconds = 4.0;
};

struct UiState {
    OverlayMode activeOverlay = OverlayMode::None;
    UiSelection selection{};
    std::array<bool, static_cast<std::size_t>(UiLayer::Count)> enabledLayers{};
    bool showDebug = false;
    bool showMetrics = true;
    bool showHud = true;
    bool showGeoGrid = true;
    bool scenarioDroplistOpen = false;
    bool objectivesDroplistOpen = false;
    int requestedScenarioIndex = -1;
    bool timelineCategoryDroplistOpen = false;
    bool timelineFilterDroplistOpen = false;
    TimelineCategory timelineCategory = TimelineCategory::All;
    TimelineFilter timelineFilter = TimelineFilter::RecentFirst;
    bool placementActive = false;
    TopologyMutationType activeMutation = TopologyMutationType::AddCache;
    int placementCandidateIndex = 0;
    int hoveredActionIndex = -1;
    int selectedActionIndex = -1;
    std::string latestFeedback;
    std::deque<ActionFeedback> actionHistory;
    std::deque<MetricsSnapshot> metricsHistory;
    double lastMetricSampleTime = -1.0;

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
const char* timelineCategoryName(TimelineCategory category);
const char* timelineFilterName(TimelineFilter filter);
