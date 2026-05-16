#include "control/InterventionController.hpp"

#include <algorithm>

void InterventionController::handleActions(std::span<const InputEvent> events, Simulation& simulation, UiState& uiState)
{
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        switch (event.action) {
        case InputAction::ScaleUp:
            mechanicExecutor_.execute(simulation, {MechanicType::ScaleUp, -1, 1.5});
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
            mechanicExecutor_.execute(simulation, {MechanicType::EnableCache});
            break;
        case InputAction::ClearCache:
            mechanicExecutor_.execute(simulation, {MechanicType::ClearCache});
            break;
        case InputAction::ToggleRetries:
            mechanicExecutor_.execute(simulation, {MechanicType::ToggleRetries});
            break;
        case InputAction::ToggleTrafficBurst:
            simulation.toggleBurstMode();
            break;
        case InputAction::ResetInterventions:
            simulation.resetProcessingCapacity();
            break;
        case InputAction::EnableTracing:
            mechanicExecutor_.execute(simulation, {MechanicType::EnableTracing});
            break;
        case InputAction::ThrottleTrafficUp:
            mechanicExecutor_.execute(simulation, {MechanicType::ThrottleTraffic, -1, 1.0});
            break;
        case InputAction::ThrottleTrafficDown:
            mechanicExecutor_.execute(simulation, {MechanicType::ThrottleTraffic, -1, -1.0});
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
        uiState.placementActive = false;
    }
}
