#include "app/Application.hpp"

#include "content/ContentRegistry.hpp"

#include "raylib.h"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr double kFixedStepSeconds = 1.0 / 60.0;

Simulation makeSimulation(const ScenarioDefinition& scenario)
{
    return Simulation(scenario, content::ContentRegistry::instance().simulationConfig());
}

std::string capacityUsageSummary(const UiState& state)
{
    std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> domainUsage{};
    int total = 0;
    for (const auto& planned : state.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            domainUsage[static_cast<std::size_t>(cost.domain)] += cost.amount;
            total += cost.amount;
        }
    }
    if (total == 0) {
        return "0/" + std::to_string(state.engineeringCapacity.total);
    }

    std::string summary = std::to_string(total) + "/" + std::to_string(state.engineeringCapacity.total) + " total";
    for (std::size_t index = 0; index < domainUsage.size(); ++index) {
        if (domainUsage[index] <= 0) {
            continue;
        }
        const auto domain = static_cast<EngineeringDomain>(index);
        summary += ", ";
        summary += engineeringDomainName(domain);
        summary += " ";
        summary += std::to_string(domainUsage[index]);
    }
    return summary;
}

EngineeringCapacity scaledCapacityBonus(EngineeringCapacity bonus, double intensity)
{
    auto scale = [intensity](int value) {
        return static_cast<int>(std::round(static_cast<double>(value) * intensity));
    };
    bonus.frontend = scale(bonus.frontend);
    bonus.backend = scale(bonus.backend);
    bonus.infrastructure = scale(bonus.infrastructure);
    bonus.data = scale(bonus.data);
    bonus.operations = scale(bonus.operations);
    bonus.total = scale(bonus.total);
    return bonus;
}

EngineeringCapacity addCapacity(EngineeringCapacity base, const EngineeringCapacity& bonus)
{
    base.frontend += bonus.frontend;
    base.backend += bonus.backend;
    base.infrastructure += bonus.infrastructure;
    base.data += bonus.data;
    base.operations += bonus.operations;
    base.total += bonus.total;
    return base;
}

double deterministicIntensity(const std::string& id, std::uint32_t seed, double minValue, double maxValue)
{
    const std::size_t hash = std::hash<std::string>{}(id) ^ (static_cast<std::size_t>(seed) * 0x9e3779b9U);
    const double t = static_cast<double>(hash % 1000U) / 999.0;
    return minValue + (maxValue - minValue) * t;
}

double deterministicRange(const std::string& id, std::uint32_t seed, const NumericRange& range)
{
    return deterministicIntensity(id, seed, range.min, range.max);
}

EngineeringCapacity sampledCapacityBonus(const content::WorldActionDefinition& definition, std::uint32_t seed)
{
    auto sample = [&](const char* field, const NumericRange& range) {
        return static_cast<int>(std::round(deterministicRange(definition.id + field, seed, range)));
    };
    return {
        .frontend = sample(".capacity.frontend", definition.frontendCapacityBonusRange),
        .backend = sample(".capacity.backend", definition.backendCapacityBonusRange),
        .infrastructure = sample(".capacity.infrastructure", definition.infrastructureCapacityBonusRange),
        .data = sample(".capacity.data", definition.dataCapacityBonusRange),
        .operations = sample(".capacity.operations", definition.operationsCapacityBonusRange),
        .total = sample(".capacity.total", definition.totalCapacityBonusRange),
    };
}
}

Application::Application()
    : scenarioDefinition_(Scenario::createDefault()),
      scenarioManager_(scenarioDefinition_),
      simulation_(makeSimulation(scenarioManager_.definition())),
      renderer_(scenarioManager_.definition())
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(kWindowWidth, kWindowHeight, "Infra Sandbox");
    SetTargetFPS(120);
    for (const auto& error : content::ContentRegistry::instance().loadErrors()) {
        TraceLog(LOG_WARNING, "Content: %s", error.c_str());
    }
}

Application::~Application()
{
    renderer_.releaseResources();
    CloseWindow();
}

void Application::run()
{
    while (!WindowShouldClose()) {
        handleInput();
        cameraController_.update(GetFrameTime());
        updatePhaseSimulation(GetFrameTime());

        renderer_.draw(simulation_, scenarioManager_, paused_, cameraController_);
    }
}

void Application::handleInput()
{
    applyPendingScenarioSelection();
    applySandboxRequests();
    applyUiRequests();

    const auto events = inputManager_.poll();
    const auto simulationResult = simulationController_.handleActions(events, simulation_, paused_);
    if (simulationResult.resetRequested) {
        resetScenario();
    }

    interventionController_.handleActions(events, simulation_, scenarioManager_, renderer_.uiManager().state());
    const UiState& uiState = renderer_.uiManager().state();
    if (!(uiState.gameplayPhase == GameplayPhase::Planning && uiState.worldActionDraftVisible && !uiState.worldActionDraft.empty())) {
        cameraController_.handleActions(events, GetFrameTime());
    }
    overlayController_.handleActions(events, renderer_.uiManager().state());
    selectionController_.handleActions(events, simulation_, cameraController_, renderer_.uiManager().state());
    uiController_.handleActions(events, renderer_.uiManager().state());

    for (const auto& event : events) {
        if (event.action == InputAction::ResetSimulation && event.phase == InputPhase::Pressed) {
            fixedStepAccumulator_ = 0.0;
        }
    }

    applyPendingScenarioSelection();
    applySandboxRequests();
    applyUiRequests();
}

void Application::resetScenario()
{
    scenarioManager_.reset();
    simulation_ = makeSimulation(scenarioManager_.definition());
    UiState& state = renderer_.uiManager().state();
    state.gameplayPhase = GameplayPhase::Observation;
    state.plannedInterventions.clear();
    clearWorldActionPlan();
    state.resolutionSummaries.clear();
    state.lastCapacityUsageSummary.clear();
    state.transitionActionsApplied = false;
    state.transitionVisualElapsedSeconds = 0.0;
    state.transitionSimulatedSeconds = 0.0;
    fixedStepAccumulator_ = 0.0;
}

void Application::loadScenario(std::size_t scenarioIndex)
{
    const std::vector<ScenarioDefinition> scenarios = ScenarioRegistry::createAll();
    if (scenarioIndex >= scenarios.size()) {
        return;
    }

    scenarioDefinition_ = scenarios[scenarioIndex];
    scenarioManager_.load(scenarioDefinition_);
    simulation_ = makeSimulation(scenarioManager_.definition());

    UiState& state = renderer_.uiManager().state();
    state.selection = {};
    state.latestFeedback.clear();
    state.actionHistory.clear();
    state.scenarioDroplistOpen = false;
    state.objectivesDroplistOpen = false;
    state.gameplayPhase = GameplayPhase::Observation;
    state.plannedInterventions.clear();
    clearWorldActionPlan();
    state.resolutionSummaries.clear();
    state.lastCapacityUsageSummary.clear();
    state.transitionActionsApplied = false;
    fixedStepAccumulator_ = 0.0;
}

void Application::applyPendingScenarioSelection()
{
    UiState& state = renderer_.uiManager().state();
    if (state.requestedScenarioIndex < 0) {
        return;
    }

    const int scenarioIndex = state.requestedScenarioIndex;
    state.requestedScenarioIndex = -1;
    loadScenario(static_cast<std::size_t>(scenarioIndex));
}

void Application::applySandboxRequests()
{
    UiState& state = renderer_.uiManager().state();
    if (!scenarioManager_.definition().sandboxLab) {
        return;
    }
    scenarioManager_.setSandboxTrafficMultiplier(state.sandboxTrafficMultiplier);
    scenarioManager_.setSandboxLatencyMultiplier(state.sandboxLatencyMultiplier);
    scenarioManager_.setSandboxQueueBuildup(state.sandboxQueueBuildup);
    if (!state.sandboxEventRequest.empty()) {
        scenarioManager_.injectSandboxEvent(state.sandboxEventRequest, simulation_);
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
        scenarioManager_.clearSandboxEvents();
        state.sandboxClearTimelineRequested = false;
    }
    if (state.sandboxSlowMotionRequested) {
        state.transitionPlaybackScale = 6.0;
        beginTransition();
        state.sandboxSlowMotionRequested = false;
    }
    if (state.sandboxStepRequested) {
        state.transitionPlaybackScale = 10.0;
        beginTransition();
        state.sandboxStepRequested = false;
    }
    if (state.sandboxResetSimulationRequested || state.sandboxRestoreTopologyRequested) {
        resetScenario();
        state.sandboxResetSimulationRequested = false;
        state.sandboxRestoreTopologyRequested = false;
    }
    if (state.sandboxRegenerateRequested) {
        scenarioManager_.setSandboxSeed(static_cast<std::uint32_t>(state.sandboxSeed));
        simulation_ = makeSimulation(scenarioManager_.definition());
        fixedStepAccumulator_ = 0.0;
        state.sandboxRegenerateRequested = false;
    }
}

void Application::applyUiRequests()
{
    UiState& state = renderer_.uiManager().state();
    if (state.phaseAdvanceRequested) {
        state.phaseAdvanceRequested = false;
        if (state.gameplayPhase == GameplayPhase::Observation) {
            state.gameplayPhase = GameplayPhase::Planning;
            state.lastCapacityUsageSummary.clear();
            generateWorldActionDraft();
            state.latestFeedback = "Planning phase. Queue actions, then validate the plan.";
        } else if (state.gameplayPhase == GameplayPhase::Planning) {
            if (!state.worldActionDraft.empty() && state.selectedWorldActionIndex < 0) {
                state.worldActionDraftVisible = true;
                state.latestFeedback = "Pick a World Action before validating the plan.";
                return;
            }
            state.transitionPlaybackScale = 24.0;
            beginTransition();
        } else if (state.gameplayPhase == GameplayPhase::Resolution) {
            state.gameplayPhase = GameplayPhase::Observation;
            state.resolutionSummaries.clear();
            state.lastCapacityUsageSummary.clear();
            clearWorldActionPlan();
            state.latestFeedback = "Observation phase. Inspect pressure movement before planning again.";
        }
    }
    if (state.fullscreenToggleRequested) {
        if (IsWindowMaximized()) {
            RestoreWindow();
        } else {
            MaximizeWindow();
        }
        state.fullscreenToggleRequested = false;
    }
}

void Application::beginTransition()
{
    UiState& state = renderer_.uiManager().state();
    const GameplayDuration& duration = scenarioManager_.currentTransitionDuration();
    state.transitionTargetSimulatedSeconds = duration.simulationSeconds;
    state.transitionDurationLabel = duration.label.empty()
        ? std::string("platform evolution")
        : duration.label;
    const double calendarDays = gameplayDurationCalendarDays(duration);
    scenarioManager_.setCalendarProgressionScale(duration.simulationSeconds > 0.0 ? calendarDays / duration.simulationSeconds : 0.0);
    state.gameplayPhase = GameplayPhase::Transition;
    state.transitionActionsApplied = false;
    state.transitionVisualElapsedSeconds = 0.0;
    state.transitionSimulatedSeconds = 0.0;
    state.resolutionSummaries.clear();
    applyWorldActionPlan();
    state.latestFeedback = "Transition running. Simulating " + state.transitionDurationLabel + ".";
    transitionBaseline_ = simulation_.metrics();
    fixedStepAccumulator_ = 0.0;
}

void Application::updatePhaseSimulation(float frameTime)
{
    UiState& state = renderer_.uiManager().state();
    if (state.gameplayPhase != GameplayPhase::Transition) {
        simulation_.setPaused(true);
        return;
    }

    simulation_.setPaused(false);
    if (!state.transitionActionsApplied) {
        applyPlannedInterventions();
        state.transitionActionsApplied = true;
    }

    state.transitionVisualElapsedSeconds += frameTime;
    const double remaining = std::max(0.0, state.transitionTargetSimulatedSeconds - state.transitionSimulatedSeconds);
    const double simulatedDelta = std::min(remaining, static_cast<double>(frameTime) * state.transitionPlaybackScale);
    fixedStepAccumulator_ += simulatedDelta;
    while (fixedStepAccumulator_ >= kFixedStepSeconds && state.transitionSimulatedSeconds < state.transitionTargetSimulatedSeconds) {
        scenarioManager_.update(kFixedStepSeconds, simulation_);
        simulation_.update(kFixedStepSeconds);
        fixedStepAccumulator_ -= kFixedStepSeconds;
        state.transitionSimulatedSeconds += kFixedStepSeconds;
    }

    if (state.transitionSimulatedSeconds >= state.transitionTargetSimulatedSeconds || state.transitionVisualElapsedSeconds >= 5.0) {
        finishTransition(transitionBaseline_);
    }
}

void Application::applyPlannedInterventions()
{
    UiState& state = renderer_.uiManager().state();
    MechanicExecutor executor;
    TopologyBuilder topologyBuilder;
    const std::string worldActionSummary = state.lastCapacityUsageSummary;
    state.lastCapacityUsageSummary = capacityUsageSummary(state);
    if (!worldActionSummary.empty()) {
        state.lastCapacityUsageSummary = state.lastCapacityUsageSummary.empty()
            ? worldActionSummary
            : state.lastCapacityUsageSummary + "; " + worldActionSummary;
    }
    for (const auto& planned : state.plannedInterventions) {
        if (planned.kind == PlannedInterventionKind::Mechanic) {
            executor.execute(simulation_, planned.command);
            scenarioManager_.notifyActionTriggered(planned.command.type);
            state.pendingVisualFeedbackEvents.push_back({
                .kind = planned.command.type == MechanicType::ThrottleTraffic ? VisualFeedbackKind::TrafficShift : VisualFeedbackKind::ActionAcknowledged,
                .targetNodeId = planned.command.targetId,
                .mechanic = planned.command.type,
                .label = planned.actionName,
            });
        } else {
            if (topologyBuilder.apply(simulation_, planned.mutation)) {
                const MechanicType mechanic = planned.mutationType == TopologyMutationType::AddCache ? MechanicType::AddCache
                    : planned.mutationType == TopologyMutationType::AddReadReplica ? MechanicType::AddReadReplica
                    : planned.mutationType == TopologyMutationType::AddQueue ? MechanicType::AddQueue
                    : MechanicType::AddRegionalCache;
                scenarioManager_.notifyActionTriggered(mechanic);
                state.pendingVisualFeedbackEvents.push_back({
                    .kind = VisualFeedbackKind::TopologyMutation,
                    .targetNodeId = -1,
                    .targetLinkId = -1,
                    .mechanic = mechanic,
                    .mutation = planned.mutationType,
                    .label = planned.actionName,
                });
            }
        }
        state.actionHistory.push_back({simulation_.timeSeconds(), planned.actionName, planned.target, planned.preview, simulation_.metrics(), true, false, 4.0});
    }
    state.plannedInterventions.clear();
    clearWorldActionPlan();
}

void Application::generateWorldActionDraft()
{
    UiState& state = renderer_.uiManager().state();
    if (!state.worldActionDraft.empty()) {
        return;
    }

    std::vector<content::WorldActionDefinition> candidates = content::ContentRegistry::instance().worldActions();
    const PressureCategory dominant = simulation_.pressure().dominantPressure;
    std::stable_sort(candidates.begin(), candidates.end(), [dominant](const auto& lhs, const auto& rhs) {
        const bool lhsMatch = std::find(lhs.affectedPressures.begin(), lhs.affectedPressures.end(), dominant) != lhs.affectedPressures.end();
        const bool rhsMatch = std::find(rhs.affectedPressures.begin(), rhs.affectedPressures.end(), dominant) != rhs.affectedPressures.end();
        return lhsMatch && !rhsMatch;
    });

    const std::size_t maxDraft = std::min<std::size_t>(3, candidates.size());
    for (std::size_t i = 0; i < maxDraft; ++i) {
        const auto& definition = candidates[i];
        const std::uint32_t seed = scenarioManager_.run().seed + static_cast<std::uint32_t>(scenarioManager_.elapsedSeconds());
        const double intensity = deterministicIntensity(definition.id, seed, definition.minIntensity, definition.maxIntensity);
        state.worldActionDraft.push_back({
            .id = definition.id,
            .name = definition.displayName,
            .description = definition.description,
            .category = definition.categories.empty() ? "World" : definition.categories.front(),
            .usefulWhen = definition.usefulWhen,
            .tradeOff = definition.tradeoffs,
            .iconId = definition.iconId,
            .capacityBonus = scaledCapacityBonus(sampledCapacityBonus(definition, seed), intensity),
            .intensity = intensity,
            .pressureResistance = deterministicRange(definition.id + ".pressure_resistance", seed, definition.pressureResistanceRange) * intensity,
            .eventIntensityMultiplier = deterministicRange(definition.id + ".event_intensity_multiplier", seed, definition.eventIntensityMultiplierRange),
            .complexityDelta = deterministicRange(definition.id + ".complexity_delta", seed, definition.complexityDeltaRange) * intensity,
            .durationSeconds = deterministicRange(definition.id + ".duration_seconds", seed, definition.durationSecondsRange),
        });
    }
    state.selectedWorldActionIndex = -1;
    state.hoveredWorldActionIndex = -1;
    state.worldActionCapacityBonus = {};
    state.worldActionDraftVisible = !state.worldActionDraft.empty();
}

void Application::clearWorldActionPlan()
{
    UiState& state = renderer_.uiManager().state();
    state.worldActionDraft.clear();
    state.worldActionDraftVisible = false;
    state.selectedWorldActionIndex = -1;
    state.hoveredWorldActionIndex = -1;
    state.worldActionCapacityBonus = {};
}

void Application::applyWorldActionPlan()
{
    UiState& state = renderer_.uiManager().state();
    if (state.selectedWorldActionIndex < 0 || state.selectedWorldActionIndex >= static_cast<int>(state.worldActionDraft.size())) {
        return;
    }
    const WorldActionDraft& action = state.worldActionDraft[static_cast<std::size_t>(state.selectedWorldActionIndex)];
    if (action.complexityDelta > 0.0) {
        simulation_.addComplexity(action.complexityDelta);
    }
    state.actionHistory.push_back({simulation_.timeSeconds(), action.name, "World", action.description, simulation_.metrics(), false, true, 0.0});
    state.lastCapacityUsageSummary = state.lastCapacityUsageSummary.empty()
        ? "World action: " + action.name
        : state.lastCapacityUsageSummary + "; world action: " + action.name;
}

void Application::finishTransition(const MetricsSnapshot& beforeMetrics)
{
    UiState& state = renderer_.uiManager().state();
    const MetricsSnapshot after = simulation_.metrics();
    state.gameplayPhase = GameplayPhase::Resolution;
    state.transitionActionsApplied = false;
    appendResolutionSummary("Simulated " + state.transitionDurationLabel + ".");
    if (!state.lastCapacityUsageSummary.empty()) {
        appendResolutionSummary("Engineering capacity used: " + state.lastCapacityUsageSummary + ".");
    }
    if (after.apiQueueDepth < beforeMetrics.apiQueueDepth) {
        appendResolutionSummary("API queue pressure improved locally.");
    } else if (after.apiQueueDepth > beforeMetrics.apiQueueDepth) {
        appendResolutionSummary("API queue pressure increased during the transition.");
    }
    if (after.databaseQueueDepth > beforeMetrics.databaseQueueDepth) {
        appendResolutionSummary("Persistence pressure increased downstream.");
    } else if (after.databaseQueueDepth < beforeMetrics.databaseQueueDepth) {
        appendResolutionSummary("Persistence pressure decreased after the plan.");
    }
    if (after.averageLatencySeconds > beforeMetrics.averageLatencySeconds * 1.1) {
        appendResolutionSummary("Average latency worsened; inspect dependency paths.");
    } else if (after.averageLatencySeconds + 0.01 < beforeMetrics.averageLatencySeconds) {
        appendResolutionSummary("Latency improved over the transition window.");
    }
    if (simulation_.pressure().dominantPressure != PressureCategory::None) {
        appendResolutionSummary(std::string("Emerging bottleneck: ") + pressureCategoryName(simulation_.pressure().dominantPressure) + " pressure.");
    }
    // The resolution panel already renders resolutionSummaries. Keep latestFeedback
    // as a compact status message so the UI does not show overlapping/duplicate
    // transition result lines.
    state.latestFeedback = state.resolutionSummaries.empty()
        ? "Transition complete."
        : "Transition complete. Review the resolution summary.";
    fixedStepAccumulator_ = 0.0;
}

void Application::appendResolutionSummary(std::string summary)
{
    UiState& state = renderer_.uiManager().state();
    state.resolutionSummaries.push_back(std::move(summary));
    while (state.resolutionSummaries.size() > 5) {
        state.resolutionSummaries.pop_front();
    }
}
