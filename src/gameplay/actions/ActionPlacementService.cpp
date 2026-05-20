#include "gameplay/actions/ActionPlacementService.hpp"

#include "gameplay/actions/ActionFeedback.hpp"
#include "gameplay/actions/ActionRules.hpp"

#include <string>

namespace gameplay::actions {

void ActionPlacementService::startPlacement(const Simulation& simulation, UiState& uiState, TopologyMutationType type) const
{
    if (worldActionRequiredBeforeNodeActions(uiState)) {
        uiState.latestFeedback = nodeActionGateMessage(uiState);
        return;
    }
    const MechanicType mechanic = mechanicForMutation(type);
    if (!simulation.isMechanicAllowed(mechanic)) {
        return;
    }
    if (const auto* intervention = interventionFor(mechanic); intervention != nullptr && !simulation.hasAnyRegionCapacity(intervention->regionSlotUsage)) {
        uiState.latestFeedback = "No regional deployment slots are available for this action.";
        return;
    }
    const std::vector<EngineeringCost> costs = engineeringCostsFor(mechanic);
    std::string capacityReason;
    if (!canQueueEngineeringCosts(uiState, costs, capacityReason)) {
        uiState.latestFeedback = capacityReason;
        return;
    }
    uiState.placementActive = true;
    uiState.activeMutation = type;
    uiState.placementCandidateIndex = 0;
    uiState.latestFeedback = std::string(topologyMutationName(type)) + " selected. Hover the map to preview a region, then click to queue placement.";
}

void ActionPlacementService::moveCandidate(const Simulation& simulation, UiState& uiState, int delta) const
{
    if (!uiState.placementActive) {
        return;
    }
    const PlacementCandidateGenerator candidateGenerator;
    const auto candidates = candidateGenerator.generate(simulation, uiState.activeMutation);
    if (candidates.empty()) {
        uiState.placementCandidateIndex = 0;
        return;
    }
    const int count = static_cast<int>(candidates.size());
    uiState.placementCandidateIndex = (uiState.placementCandidateIndex + delta + count) % count;
}

void ActionPlacementService::confirmPlacement(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const PlacementOption& option) const
{
    if (!uiState.placementActive) {
        return;
    }

    const MutationValidator mutationValidator;
    MutationPreview preview = mutationValidator.preview(simulation, uiState.activeMutation, option);
    const MechanicType mechanic = mechanicForMutation(uiState.activeMutation);
    if (const auto* intervention = interventionFor(mechanic)) {
        preview.mutation.complexityCost = intervention->complexityCost;
        preview.mutation.regionSlotUsage = intervention->regionSlotUsage;
        if (!simulation.canUseRegionSlots(preview.mutation.placement.location.regionName, intervention->regionSlotUsage)) {
            uiState.latestFeedback = preview.mutation.placement.displayName + " has no free deployment slots for this action.";
            return;
        }
    }
    if (!preview.valid) {
        uiState.latestFeedback = preview.validationMessage.empty() ? "This node cannot be placed here." : preview.validationMessage;
        return;
    }

    const std::string feedback = feedbackForTopologyMutation(uiState.activeMutation, option.displayName);
    const ActionQueue queue;
    queue.queueTopologyMutation(simulation, uiState, preview.mutation, uiState.activeMutation, topologyMutationName(uiState.activeMutation), option.displayName, feedback);
    uiState.placementActive = false;
    (void)scenarioManager;
}

}
