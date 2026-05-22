#include "gameplay/GameplayPhaseController.hpp"
#include "gameplay/Scenario.hpp"

#include <algorithm>
#include <string>

namespace {
constexpr double kFixedStepSeconds = 1.0 / 60.0;
}

void GameplayPhaseController::reset()
{
    fixedStepAccumulator_ = 0.0;
    transitionBaseline_ = {};
    transitionEventLogStart_ = 0;
    transitionReturnsToObservation_ = false;
}

void GameplayPhaseController::prepareScenarioGrounding(UiState& state, std::string feedback)
{
    transitionReturnsToObservation_ = true;
    state.gameplayPhase = GameplayPhase::Planning;
    state.transitionPlaybackScale = 24.0;
    state.transitionDurationLabel = "initial operational cycle";
    state.latestFeedback = feedback.empty()
        ? "Scenario loaded. Press the phase button to run the initial operational cycle."
        : std::move(feedback);
}

void GameplayPhaseController::beginScenarioGroundingSimulation(UiState& state, ScenarioSession& session, const WorldActionController& worldActions)
{
    prepareScenarioGrounding(state);
    beginTransition(state, session, worldActions);
    state.transitionDurationLabel = "initial operational cycle";
    state.latestFeedback = "Running the initial operational cycle to establish traffic, queues, and pressure.";
}

void GameplayPhaseController::applyUiRequests(UiState& state, ScenarioSession& session, const WorldActionController& worldActions)
{
    if (state.phaseAdvanceRequested) {
        state.phaseAdvanceRequested = false;
        if (state.gameplayPhase == GameplayPhase::Planning) {
            if (state.eventPopupMode != EventPopupMode::None) {
                state.latestFeedback = "Review the event briefing before resolving the turn.";
                return;
            }
            if (!state.worldActionDraft.empty() && state.selectedWorldActionIndex < 0) {
                state.worldActionDraftVisible = true;
                state.latestFeedback = "Pick a World Action before resolving the turn.";
                return;
            }
            state.transitionPlaybackScale = 24.0;
            beginTransition(state, session, worldActions);
        } else if (state.gameplayPhase == GameplayPhase::Analysis) {
            if (state.eventPopupMode != EventPopupMode::None) {
                state.latestFeedback = "Review the event recap before continuing.";
                return;
            }
            state.gameplayPhase = GameplayPhase::Planning;
            state.resolutionSummaries.clear();
            state.lastCapacityUsageSummary.clear();
            worldActions.clearPlan(state);
            state.eventPopupMode = EventPopupMode::None;
            state.eventPopupEvents.clear();
            if (auto planningEvent = session.scenarioManager().rollPlanningEvent(session.simulation())) {
                state.eventPopupEvents.push_back(*planningEvent);
                state.eventPopupMode = EventPopupMode::PlanningStart;
                state.eventPanelVisible = true;
            } else {
                state.eventPanelVisible = false;
            }
            state.eventPanelAcknowledged = false;
            worldActions.generateDraft(state, session);
            state.worldActionDraftVisible = state.eventPopupMode == EventPopupMode::None && !state.worldActionDraft.empty();
            state.latestFeedback = "Planning phase. Queue actions, then resolve one operational cycle.";
        }
    }
}

void GameplayPhaseController::beginTransition(UiState& state, ScenarioSession& session, const WorldActionController& worldActions)
{
    const GameplayDuration& duration = session.scenarioManager().currentTransitionDuration();
    state.transitionTargetSimulatedSeconds = duration.simulationSeconds;
    state.transitionDurationLabel = transitionReturnsToObservation_
        ? std::string("initial operational cycle")
        : (duration.label.empty() ? std::string("platform evolution") : duration.label);
    const double calendarDays = gameplayDurationCalendarDays(duration);
    session.scenarioManager().setCalendarProgressionScale(duration.simulationSeconds > 0.0 ? calendarDays / duration.simulationSeconds : 0.0);
    session.scenarioManager().beginTurn();
    state.gameplayPhase = GameplayPhase::Resolving;
    state.transitionActionsApplied = false;
    state.transitionVisualElapsedSeconds = 0.0;
    state.transitionSimulatedSeconds = 0.0;
    state.resolutionSummaries.clear();
    transitionEventLogStart_ = session.scenarioManager().eventLogSize();
    worldActions.applyPlan(state, session);
    state.latestFeedback = "Resolving turn " + std::to_string(session.scenarioManager().turnNumber()) + ".";
    transitionBaseline_ = session.simulation().metrics();
    fixedStepAccumulator_ = 0.0;
}

void GameplayPhaseController::updateSimulation(float frameTime, UiState& state, ScenarioSession& session, const WorldActionController& worldActions)
{
    if (state.gameplayPhase != GameplayPhase::Resolving) {
        session.simulation().setPaused(true);
        return;
    }

    session.simulation().setPaused(false);
    if (!state.transitionActionsApplied) {
        worldActions.applyPlannedInterventions(state, session);
        state.transitionActionsApplied = true;
    }

    state.transitionVisualElapsedSeconds += frameTime;

    const double target = state.transitionTargetSimulatedSeconds;
    const double remaining = target - state.transitionSimulatedSeconds;

    if (remaining <= 0.0) {
        state.transitionSimulatedSeconds = target;
        fixedStepAccumulator_ = 0.0;
        finishTransition(state, session, transitionBaseline_, worldActions);
        return;
    }

    const double simulatedDelta =
        std::min(remaining, static_cast<double>(frameTime) * state.transitionPlaybackScale);

    fixedStepAccumulator_ += simulatedDelta;

    while (fixedStepAccumulator_ > 0.0) {
        const double stepRemaining = target - state.transitionSimulatedSeconds;

        if (stepRemaining <= 0.0) {
            break;
        }

        const double step = std::min({
            kFixedStepSeconds,
            fixedStepAccumulator_,
            stepRemaining
        });

        if (step <= 0.0) {
            break;
        }

        session.scenarioManager().update(step, session.simulation());
        session.simulation().update(step);

        fixedStepAccumulator_ -= step;
        state.transitionSimulatedSeconds += step;
    }

    if (state.transitionSimulatedSeconds >= target) {
        state.transitionSimulatedSeconds = target;
        fixedStepAccumulator_ = 0.0;
        finishTransition(state, session, transitionBaseline_, worldActions);
    }
}

void GameplayPhaseController::finishTransition(UiState& state, ScenarioSession& session, const MetricsSnapshot& beforeMetrics, const WorldActionController& worldActions)
{
    const MetricsSnapshot after = session.simulation().metrics();
    state.gameplayPhase = GameplayPhase::Analysis;
    state.transitionActionsApplied = false;
    state.eventPopupEvents = session.scenarioManager().eventsSince(transitionEventLogStart_);
    state.eventPopupMode = state.eventPopupEvents.empty() ? EventPopupMode::None : EventPopupMode::SimulationRecap;
    state.eventPanelVisible = state.eventPopupMode != EventPopupMode::None;
    state.eventPanelAcknowledged = false;
    appendResolutionSummary(state, "Turn " + std::to_string(session.scenarioManager().turnNumber()) + " resolved.");
    if (!state.lastCapacityUsageSummary.empty()) {
        appendResolutionSummary(state, "Engineering capacity used: " + state.lastCapacityUsageSummary + ".");
    }
    if (after.apiQueueDepth < beforeMetrics.apiQueueDepth) {
        appendResolutionSummary(state, "API queue pressure improved locally.");
    } else if (after.apiQueueDepth > beforeMetrics.apiQueueDepth) {
        appendResolutionSummary(state, "API queue pressure increased this turn.");
    }
    if (after.databaseQueueDepth > beforeMetrics.databaseQueueDepth) {
        appendResolutionSummary(state, "Persistence pressure increased downstream this turn.");
    } else if (after.databaseQueueDepth < beforeMetrics.databaseQueueDepth) {
        appendResolutionSummary(state, "Persistence pressure decreased after the plan.");
    }
    if (after.averageLatencySeconds > beforeMetrics.averageLatencySeconds * 1.1) {
        appendResolutionSummary(state, "Average latency worsened; inspect dependency paths.");
    } else if (after.averageLatencySeconds + 0.01 < beforeMetrics.averageLatencySeconds) {
        appendResolutionSummary(state, "Latency improved this turn.");
    }
    if (session.simulation().pressure().dominantPressure != PressureCategory::None) {
        appendResolutionSummary(state, std::string("Emerging bottleneck: ") + pressureCategoryName(session.simulation().pressure().dominantPressure) + " pressure.");
    }
    state.latestFeedback = state.resolutionSummaries.empty()
        ? "Turn resolved."
        : "Turn resolved. Review the analysis summary.";

    if (transitionReturnsToObservation_) {
        transitionReturnsToObservation_ = false;
        state.gameplayPhase = GameplayPhase::Planning;
        state.eventPopupMode = EventPopupMode::None;
        state.eventPopupEvents.clear();
        state.eventPanelVisible = false;
        state.eventPanelAcknowledged = false;
        state.resolutionSummaries.clear();
        state.lastCapacityUsageSummary.clear();
        worldActions.clearPlan(state);
        if (auto planningEvent = session.scenarioManager().rollPlanningEvent(session.simulation())) {
            state.eventPopupEvents.push_back(*planningEvent);
            state.eventPopupMode = EventPopupMode::PlanningStart;
            state.eventPanelVisible = true;
        }
        state.eventPanelAcknowledged = false;
        worldActions.generateDraft(state, session);
        state.worldActionDraftVisible = state.eventPopupMode == EventPopupMode::None && !state.worldActionDraft.empty();
        state.latestFeedback = "Initial operational state ready. Plan the next turn.";
    }
    fixedStepAccumulator_ = 0.0;
}

void GameplayPhaseController::appendResolutionSummary(UiState& state, std::string summary) const
{
    state.resolutionSummaries.push_back(std::move(summary));
    while (state.resolutionSummaries.size() > 5) {
        state.resolutionSummaries.pop_front();
    }
}
