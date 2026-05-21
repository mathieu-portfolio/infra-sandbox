#include "gameplay/WorldActionController.hpp"

#include "gameplay/world_actions/PlannedInterventionApplier.hpp"
#include "gameplay/world_actions/WorldActionDraftService.hpp"
#include "gameplay/world_actions/WorldActionEffectApplier.hpp"

void WorldActionController::generateDraft(UiState& state, ScenarioSession& session) const
{
    WorldActionDraftService{}.generateDraft(state, session);
}

void WorldActionController::clearPlan(UiState& state) const
{
    state.eventPopupMode = EventPopupMode::None;
    state.eventPopupEvents.clear();
    state.eventPanelVisible = false;
    state.eventPanelAcknowledged = false;
    state.worldActionDraft.clear();
    state.worldActionDraftVisible = false;
    state.selectedWorldActionIndex = -1;
    state.hoveredWorldActionIndex = -1;
    state.worldActionCapacityBonus = {};
    state.previewEngineeringCapacity = {};
    state.engineeringCapacityPreviewVisible = false;
}

void WorldActionController::applyPlan(UiState& state, ScenarioSession& session) const
{
    WorldActionEffectApplier{}.applySelectedWorldAction(state, session);
}

void WorldActionController::applyPlannedInterventions(UiState& state, ScenarioSession& session) const
{
    PlannedInterventionApplier{}.apply(state, session);
    clearPlan(state);
}
