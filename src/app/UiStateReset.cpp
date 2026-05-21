#include "app/UiStateReset.hpp"

#include "ui/core/UiTypes.hpp"

void resetUiStateForScenario(UiState& state, const std::string& feedback)
{
    state.gameplayPhase = GameplayPhase::Planning;
    state.phaseAdvanceRequested = false;
    state.transitionActionsApplied = false;
    state.transitionVisualElapsedSeconds = 0.0;
    state.transitionSimulatedSeconds = 0.0;
    state.transitionTargetSimulatedSeconds = 0.0;
    state.transitionPlaybackScale = 24.0;
    state.transitionDurationLabel = "operational cycle";
    state.plannedInterventions.clear();
    state.eventPopupMode = EventPopupMode::None;
    state.eventPopupEvents.clear();
    state.eventPanelVisible = false;
    state.eventPanelAcknowledged = false;
    state.worldActionDraft.clear();
    state.worldActionDraftVisible = false;
    state.selectedWorldActionIndex = -1;
    state.hoveredWorldActionIndex = -1;
    state.suppressMapSelectionOnce = false;
    state.worldActionCapacityBonus = {};
    state.previewEngineeringCapacity = {};
    state.engineeringCapacityPreviewVisible = false;
    state.resolutionSummaries.clear();
    state.lastCapacityUsageSummary.clear();
    state.selection = {};
    state.activeNodeInspectionTab = NodeInspectionTab::Overview;
    state.observability = {};
    state.scenarioDroplistOpen = false;
    state.packDroplistOpen = false;
    state.requestedPackId.clear();
    state.objectivesDroplistOpen = false;
    state.requestedScenarioIndex = -1;
    state.requestedScenarioId.clear();
    state.timelineCategoryDroplistOpen = false;
    state.timelineFilterDroplistOpen = false;
    state.placementActive = false;
    state.activeActionId.clear();
    state.activeActionCategoryIndex = 0;
    state.nodeActionScrollOffset = 0.0f;
    state.actionUseCounts.clear();
    state.resetScenarioRequested = false;
    state.hoveredActionIndex = -1;
    state.selectedActionIndex = -1;
    state.latestFeedback = feedback;
    state.pendingVisualFeedbackEvents.clear();
    state.actionHistory.clear();
    state.metricsHistory.clear();
    state.lastMetricSampleTime = -1.0;
}
