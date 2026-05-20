#pragma once

#include "simulation/Metrics.hpp"
#include "simulation/Mechanics.hpp"
#include "simulation/TopologyMutation.hpp"
#include "gameplay/Scenario.hpp"

#include <array>
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

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


enum class UiViewMode {
    Overview,
    Traffic,
    Resources,
    Persistence,
    Reliability,
    Geography,
    Count
};

enum class EventPopupMode {
    None,
    PlanningStart,
    SimulationRecap
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


enum class NodeInspectionTab {
    Overview,
    Metrics,
    Traffic,
    Dependencies,
    Diagnostics,
    Count
};

struct ObservabilityState {
    bool metricsUnlocked = false;
    bool trafficUnlocked = false;
    bool dependenciesUnlocked = false;
    bool diagnosticsUnlocked = false;
    int telemetryDelayTurns = 0;
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

enum class GameplayPhase {
    Planning,
    Resolving,
    Analysis
};

struct UiSelection {
    int nodeId = -1;
    int requestId = -1;
    int linkId = -1;
};

enum class VisualFeedbackKind {
    ActionAcknowledged,
    TopologyMutation,
    TrafficShift,
    PressureInjected,
    Stabilization
};

struct VisualFeedbackEvent {
    VisualFeedbackKind kind = VisualFeedbackKind::ActionAcknowledged;
    int targetNodeId = -1;
    int targetLinkId = -1;
    MechanicType mechanic = MechanicType::ScaleUp;
    TopologyMutationType mutation = TopologyMutationType::AddCache;
    std::string label;
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

enum class PlannedInterventionKind {
    Mechanic,
    TopologyMutation
};

struct PlannedIntervention {
    PlannedInterventionKind kind = PlannedInterventionKind::Mechanic;
    MechanicCommand command{};
    TopologyMutation mutation{};
    TopologyMutationType mutationType = TopologyMutationType::AddCache;
    std::string actionName;
    std::string target;
    std::string preview;
    std::vector<EngineeringCost> engineeringCosts;
};

struct WorldActionDraft {
    std::string id;
    std::string name;
    std::string description;
    std::string category;
    std::string usefulWhen;
    std::string tradeOff;
    bool showUsageDetails = true;
    std::string iconId;
    EngineeringCapacity capacityBonus;
    double intensity = 1.0;
    double pressureResistance = 0.0;
    double eventIntensityMultiplier = 1.0;
    double complexityDelta = 0.0;
    double durationSeconds = 0.0;
    std::vector<std::string> unlocksObservability;
};

struct UiState {
    GameplayPhase gameplayPhase = GameplayPhase::Planning;
    bool phaseAdvanceRequested = false;
    bool transitionActionsApplied = false;
    double transitionVisualElapsedSeconds = 0.0;
    double transitionSimulatedSeconds = 0.0;
    double transitionTargetSimulatedSeconds = 0.0;
    double transitionPlaybackScale = 24.0;
    std::string transitionDurationLabel = "operational cycle";
    std::deque<PlannedIntervention> plannedInterventions;
    EventPopupMode eventPopupMode = EventPopupMode::None;
    std::vector<EventLogEntry> eventPopupEvents;
    bool eventPanelVisible = false;
    bool eventPanelAcknowledged = false;
    std::vector<WorldActionDraft> worldActionDraft;
    bool worldActionDraftVisible = false;
    int selectedWorldActionIndex = -1;
    int hoveredWorldActionIndex = -1;
    bool suppressMapSelectionOnce = false;
    EngineeringCapacity worldActionCapacityBonus;
    std::deque<std::string> resolutionSummaries;
    // Committed action-point capacities. This is the single source used by
    // validation and phase-independent UI. Hover/selection previews are kept
    // separate so they cannot be accidentally applied twice.
    EngineeringCapacity engineeringCapacity;
    EngineeringCapacity previewEngineeringCapacity;
    bool engineeringCapacityPreviewVisible = false;
    std::string lastCapacityUsageSummary;
    UiViewMode activeViewMode = UiViewMode::Overview;
    OverlayMode activeOverlay = OverlayMode::Bottlenecks;
    UiSelection selection{};
    NodeInspectionTab activeNodeInspectionTab = NodeInspectionTab::Overview;
    ObservabilityState observability{};
    std::array<bool, static_cast<std::size_t>(UiLayer::Count)> enabledLayers{};
    bool showDebug = false;
    bool showMetrics = true;
    bool showHud = true;
    bool showGeoGrid = true;
    bool optionsMenuOpen = false;
    bool fullscreenToggleRequested = false;
    bool packDroplistOpen = false;
    std::string requestedPackId;
    bool scenarioDroplistOpen = false;
    bool objectivesDroplistOpen = false;
    int requestedScenarioIndex = -1;
    std::string requestedScenarioId;
    bool timelineCategoryDroplistOpen = false;
    bool timelineFilterDroplistOpen = false;
    TimelineCategory timelineCategory = TimelineCategory::All;
    TimelineFilter timelineFilter = TimelineFilter::RecentFirst;
    bool sandboxMode = false;
    double sandboxTrafficMultiplier = 1.0;
    double sandboxLatencyMultiplier = 1.0;
    bool sandboxQueueBuildup = false;
    int sandboxSeed = 1;
    std::string sandboxEventRequest;
    bool sandboxResetSimulationRequested = false;
    bool sandboxRestoreTopologyRequested = false;
    bool sandboxClearTimelineRequested = false;
    bool sandboxRegenerateRequested = false;
    bool sandboxSlowMotionRequested = false;
    bool sandboxStepRequested = false;
    bool placementActive = false;
    TopologyMutationType activeMutation = TopologyMutationType::AddCache;
    int placementCandidateIndex = 0;
    int hoveredActionIndex = -1;
    std::vector<EngineeringCost> hoveredActionEngineeringCosts;
    int selectedActionIndex = -1;
    std::string latestFeedback;
    std::vector<VisualFeedbackEvent> pendingVisualFeedbackEvents;
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

const char* uiViewModeName(UiViewMode mode);
const char* uiViewModeIcon(UiViewMode mode);
OverlayMode overlayForViewMode(UiViewMode mode);
const char* overlayModeName(OverlayMode mode);
const char* timelineCategoryName(TimelineCategory category);
const char* timelineFilterName(TimelineFilter filter);
