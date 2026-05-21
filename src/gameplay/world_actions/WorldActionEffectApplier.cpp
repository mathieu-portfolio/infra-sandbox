#include "gameplay/world_actions/WorldActionEffectApplier.hpp"

#include "gameplay/world_actions/WorldActionPressureUtils.hpp"

namespace {
void applyObservabilityUnlocks(UiState& state, const std::vector<std::string>& unlocks)
{
    for (const auto& unlock : unlocks) {
        if (unlock == "metrics") {
            state.observability.metricsUnlocked = true;
        } else if (unlock == "traffic") {
            state.observability.trafficUnlocked = true;
        } else if (unlock == "dependencies" || unlock == "tracing") {
            state.observability.dependenciesUnlocked = true;
        } else if (unlock == "diagnostics") {
            state.observability.diagnosticsUnlocked = true;
        }
    }
}
}

void WorldActionEffectApplier::applySelectedWorldAction(UiState& state, ScenarioSession& session) const
{
    if (state.selectedWorldActionIndex < 0 || state.selectedWorldActionIndex >= static_cast<int>(state.worldActionDraft.size())) {
        return;
    }

    const WorldActionDraft& action = state.worldActionDraft[static_cast<std::size_t>(state.selectedWorldActionIndex)];
    applyObservabilityUnlocks(state, action.unlocksObservability);
    session.scenarioManager().applyEngineeringCapacityBonus(action.capacityBonus);
    state.engineeringCapacity = session.scenarioManager().definition().engineeringCapacity;
    if (gameplay::world_actions::hasPressureEffect(action.pressureEffect)) {
        session.simulation().applyPressureEffect(action.pressureEffect);
    }
    if (action.complexityDelta > 0.0) {
        // TODO: Keep this direct complexity path only for legacy content that
        // has not yet expressed its trade-off as a backend/service pressure.
        session.simulation().addComplexity(action.complexityDelta);
    }
    state.actionHistory.push_back({
        session.simulation().timeSeconds(),
        action.id,
        action.name,
        "World",
        action.description,
        session.simulation().metrics(),
        false,
        true,
        0.0,
    });
    state.lastCapacityUsageSummary = state.lastCapacityUsageSummary.empty()
        ? "World action: " + action.name
        : state.lastCapacityUsageSummary + "; world action: " + action.name;

    // Once the world action is committed into ScenarioManager, it is no longer a preview.
    // Leaving the selection active let later planning/analysis frames add the same bonus
    // on top of the newly committed capacity.
    state.selectedWorldActionIndex = -1;
    state.hoveredWorldActionIndex = -1;
    state.worldActionCapacityBonus = {};
    state.previewEngineeringCapacity = {};
    state.engineeringCapacityPreviewVisible = false;
}
