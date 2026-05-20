#include "gameplay/actions/ActionQueue.hpp"

#include "gameplay/actions/ActionRules.hpp"
#include "simulation/core/Simulation.hpp"

namespace gameplay::actions {

void ActionQueue::queueMechanic(const Simulation& simulation, UiState& uiState, const MechanicCommand& command, std::string actionName, std::string target) const
{
    if (worldActionRequiredBeforeNodeActions(uiState)) {
        uiState.latestFeedback = nodeActionGateMessage(uiState);
        return;
    }
    if (!simulation.isMechanicAllowed(command.type)) {
        uiState.latestFeedback = "This action is not available in the current scenario.";
        return;
    }
    const std::vector<EngineeringCost> costs = engineeringCostsFor(command.type);
    std::string capacityReason;
    if (!canQueueEngineeringCosts(uiState, costs, capacityReason)) {
        uiState.latestFeedback = capacityReason;
        return;
    }
    uiState.gameplayPhase = GameplayPhase::Planning;
    uiState.plannedInterventions.push_back({
        .kind = PlannedInterventionKind::Mechanic,
        .command = command,
        .actionName = std::move(actionName),
        .target = std::move(target),
        .preview = "Queued for the next turn.",
        .engineeringCosts = costs,
    });
    uiState.latestFeedback = uiState.plannedInterventions.back().actionName + " queued. Resolve the turn to see consequences.";
}

void ActionQueue::queueTopologyMutation(const Simulation&, UiState& uiState, const TopologyMutation& mutation, TopologyMutationType type, std::string actionName, std::string target, std::string preview) const
{
    if (worldActionRequiredBeforeNodeActions(uiState)) {
        uiState.latestFeedback = nodeActionGateMessage(uiState);
        return;
    }
    const MechanicType mechanic = mechanicForMutation(type);
    const std::vector<EngineeringCost> costs = engineeringCostsFor(mechanic);
    std::string capacityReason;
    if (!canQueueEngineeringCosts(uiState, costs, capacityReason)) {
        uiState.latestFeedback = capacityReason;
        return;
    }
    uiState.gameplayPhase = GameplayPhase::Planning;
    uiState.plannedInterventions.push_back({
        .kind = PlannedInterventionKind::TopologyMutation,
        .mutation = mutation,
        .mutationType = type,
        .actionName = std::move(actionName),
        .target = std::move(target),
        .preview = std::move(preview),
        .engineeringCosts = costs,
    });
    uiState.latestFeedback = uiState.plannedInterventions.back().actionName + " queued. Resolve the turn to see consequences.";
}

}
