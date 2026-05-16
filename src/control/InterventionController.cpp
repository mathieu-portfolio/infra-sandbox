#include "control/InterventionController.hpp"

#include "ui/ActionPanelModel.hpp"

#include <algorithm>
#include <string>

void InterventionController::handleActions(std::span<const InputEvent> events, Simulation& simulation, UiState& uiState)
{
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        if (event.action == InputAction::Select) {
            handleActionPanelClick(event, simulation, uiState);
            continue;
        }

        switch (event.action) {
        case InputAction::ScaleUp:
            executeMechanic(simulation, uiState, {MechanicType::ScaleUp, -1, 1.5}, "Scale Up", "API service");
            break;
        case InputAction::ScaleOut:
            mechanicExecutor_.execute(simulation, {MechanicType::ScaleOut});
            break;
        case InputAction::AddCache:
            startPlacement(simulation, uiState, TopologyMutationType::AddCache);
            break;
        case InputAction::AddReadReplica:
            startPlacement(simulation, uiState, TopologyMutationType::AddReadReplica);
            break;
        case InputAction::AddQueue:
            startPlacement(simulation, uiState, TopologyMutationType::AddQueue);
            break;
        case InputAction::AddRegionalCache:
            startPlacement(simulation, uiState, TopologyMutationType::AddRegionalCache);
            break;
        case InputAction::NextPlacementCandidate:
            moveCandidate(simulation, uiState, 1);
            break;
        case InputAction::PreviousPlacementCandidate:
            moveCandidate(simulation, uiState, -1);
            break;
        case InputAction::ConfirmPlacement:
            confirmPlacement(simulation, uiState);
            break;
        case InputAction::CancelPlacement:
            uiState.placementActive = false;
            break;
        case InputAction::ToggleCache:
            executeMechanic(simulation, uiState, {MechanicType::EnableCache}, "Toggle Cache", "Global cache behavior");
            break;
        case InputAction::ClearCache:
            executeMechanic(simulation, uiState, {MechanicType::ClearCache}, "Clear Cache", "Cache");
            break;
        case InputAction::ToggleRetries:
            executeMechanic(simulation, uiState, {MechanicType::ToggleRetries}, "Toggle Retries", "Retry policy");
            break;
        case InputAction::ToggleTrafficBurst:
            simulation.toggleBurstMode();
            break;
        case InputAction::ResetInterventions:
            simulation.resetProcessingCapacity();
            break;
        case InputAction::EnableTracing:
            executeMechanic(simulation, uiState, {MechanicType::EnableTracing}, "Enable Tracing", "Observability");
            break;
        case InputAction::ThrottleTrafficUp:
            executeMechanic(simulation, uiState, {MechanicType::ThrottleTraffic, -1, 1.0}, "Increase Traffic", "Demand");
            break;
        case InputAction::ThrottleTrafficDown:
            executeMechanic(simulation, uiState, {MechanicType::ThrottleTraffic, -1, -1.0}, "Decrease Traffic", "Demand");
            break;
        default:
            break;
        }
    }
}

void InterventionController::startPlacement(const Simulation& simulation, UiState& uiState, TopologyMutationType type) const
{
    const MechanicType mechanic = type == TopologyMutationType::AddCache ? MechanicType::AddCache
        : type == TopologyMutationType::AddReadReplica ? MechanicType::AddReadReplica
        : type == TopologyMutationType::AddQueue ? MechanicType::AddQueue
        : MechanicType::AddRegionalCache;
    if (!simulation.isMechanicAllowed(mechanic)) {
        return;
    }
    uiState.placementActive = true;
    uiState.activeMutation = type;
    uiState.placementCandidateIndex = 0;
    uiState.latestFeedback = std::string(topologyMutationName(type)) + " preview selected. Choose a region, then confirm or cancel.";
}

void InterventionController::moveCandidate(const Simulation& simulation, UiState& uiState, int delta) const
{
    if (!uiState.placementActive) {
        return;
    }
    const auto candidates = candidateGenerator_.generate(simulation, uiState.activeMutation);
    if (candidates.empty()) {
        uiState.placementCandidateIndex = 0;
        return;
    }
    const int count = static_cast<int>(candidates.size());
    uiState.placementCandidateIndex = (uiState.placementCandidateIndex + delta + count) % count;
}

void InterventionController::confirmPlacement(Simulation& simulation, UiState& uiState) const
{
    if (!uiState.placementActive) {
        return;
    }
    const auto candidates = candidateGenerator_.generate(simulation, uiState.activeMutation);
    if (candidates.empty()) {
        return;
    }
    const int index = std::clamp(uiState.placementCandidateIndex, 0, static_cast<int>(candidates.size()) - 1);
    const MutationPreview preview = mutationValidator_.preview(simulation, uiState.activeMutation, candidates[static_cast<std::size_t>(index)]);
    if (preview.valid && topologyBuilder_.apply(simulation, preview.mutation)) {
        const std::string target = candidates[static_cast<std::size_t>(index)].displayName;
        recordFeedback(uiState, simulation, topologyMutationName(uiState.activeMutation), target, std::string(topologyMutationName(uiState.activeMutation)) + " applied in " + target + ". Watch latency, queue depth, and utilization.");
        uiState.placementActive = false;
    }
}

void InterventionController::handleActionPanelClick(const InputEvent& event, Simulation& simulation, UiState& uiState) const
{
    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, uiState, GetScreenWidth(), GetScreenHeight());
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        const auto& card = cards[static_cast<std::size_t>(i)];
        if (!CheckCollisionPointRec(event.mousePosition, card.bounds)) {
            continue;
        }

        uiState.selectedActionIndex = i;
        if (!card.available) {
            uiState.latestFeedback = card.unavailableReason;
            return;
        }

        switch (card.kind) {
        case ActionCardKind::Mechanic:
            executeMechanic(simulation, uiState, {card.mechanic, uiState.selection.nodeId, 1.5}, card.name, card.target);
            break;
        case ActionCardKind::TopologyMutation:
            startPlacement(simulation, uiState, card.mutation);
            break;
        case ActionCardKind::ConfirmPreview:
            confirmPlacement(simulation, uiState);
            break;
        case ActionCardKind::CancelPreview:
            uiState.placementActive = false;
            uiState.latestFeedback = "Topology preview cancelled.";
            break;
        }
        return;
    }
}

void InterventionController::executeMechanic(Simulation& simulation, UiState& uiState, const MechanicCommand& command, std::string actionName, std::string target) const
{
    const auto before = simulation.metrics();
    mechanicExecutor_.execute(simulation, command);

    std::string message = actionName + " applied.";
    if (command.type == MechanicType::ScaleUp) {
        message = "API capacity increased. Watch queue depth and utilization.";
    } else if (command.type == MechanicType::ToggleRetries) {
        message = "Retry policy changed. Watch timeout rate and retry amplification.";
    } else if (command.type == MechanicType::ClearCache) {
        message = "Cache cleared. Repeated reads may warm it again.";
    } else if (command.type == MechanicType::EnableCache) {
        message = "Cache behavior toggled. Watch cache hit rate and DB pressure.";
    }

    uiState.latestFeedback = message;
    uiState.actionHistory.push_back({simulation.timeSeconds(), std::move(actionName), std::move(target), message, before, true, false, 4.0});
    while (uiState.actionHistory.size() > 8) {
        uiState.actionHistory.pop_front();
    }
}

void InterventionController::recordFeedback(UiState& uiState, const Simulation& simulation, std::string actionName, std::string target, std::string message) const
{
    uiState.latestFeedback = message;
    uiState.actionHistory.push_back({simulation.timeSeconds(), std::move(actionName), std::move(target), message, simulation.metrics(), true, false, 4.0});
    while (uiState.actionHistory.size() > 8) {
        uiState.actionHistory.pop_front();
    }
}
