#include "gameplay/SandboxController.hpp"

#include <cstdint>

SandboxControllerResult SandboxController::applyRequests(UiState& state, ScenarioSession& session) const
{
    SandboxControllerResult result{};
    if (!session.scenarioManager().definition().sandboxLab) {
        return result;
    }

    session.scenarioManager().setSandboxTrafficMultiplier(state.sandboxTrafficMultiplier);
    session.scenarioManager().setSandboxLatencyMultiplier(state.sandboxLatencyMultiplier);
    session.scenarioManager().setSandboxQueueBuildup(state.sandboxQueueBuildup);

    if (!state.sandboxEventRequest.empty()) {
        session.scenarioManager().injectSandboxEvent(state.sandboxEventRequest, session.simulation());
        state.latestFeedback = "Injected lab event: " + state.sandboxEventRequest;
        state.pendingVisualFeedbackEvents.push_back({
            .kind = state.sandboxEventRequest == "recovery" ? VisualFeedbackKind::Stabilization : VisualFeedbackKind::PressureInjected,
            .label = state.sandboxEventRequest,
        });
        state.sandboxEventRequest.clear();
    }
    if (state.sandboxClearTimelineRequested) {
        state.actionHistory.clear();
        state.latestFeedback.clear();
        session.scenarioManager().clearSandboxEvents();
        state.sandboxClearTimelineRequested = false;
    }
    if (state.sandboxSlowMotionRequested) {
        state.transitionPlaybackScale = 6.0;
        result.beginTransitionRequested = true;
        state.sandboxSlowMotionRequested = false;
    }
    if (state.sandboxStepRequested) {
        state.transitionPlaybackScale = 10.0;
        result.beginTransitionRequested = true;
        state.sandboxStepRequested = false;
    }
    if (state.sandboxResetSimulationRequested || state.sandboxRestoreTopologyRequested) {
        result.resetScenarioRequested = true;
        state.sandboxResetSimulationRequested = false;
        state.sandboxRestoreTopologyRequested = false;
    }
    if (state.sandboxRegenerateRequested) {
        session.scenarioManager().setSandboxSeed(static_cast<std::uint32_t>(state.sandboxSeed));
        session.regenerateSimulation();
        result.resetTransitionStateRequested = true;
        state.sandboxRegenerateRequested = false;
    }

    return result;
}
