#include "gameplay/actions/ActionPlacementService.hpp"

#include "gameplay/actions/ActionFeedback.hpp"
#include "gameplay/actions/ActionRules.hpp"
#include "rendering/RenderPrimitives.hpp"
#include "ui/core/UiLayout.hpp"

#include <string>

namespace {
Rectangle mapBounds(const CameraController& camera, int screenWidth, int screenHeight)
{
    const Vector2 topLeft = worldToScreen({-MapProjection::worldWidth * 0.5f, -MapProjection::worldHeight * 0.5f}, screenWidth, screenHeight, camera);
    const Vector2 bottomRight = worldToScreen({MapProjection::worldWidth * 0.5f, MapProjection::worldHeight * 0.5f}, screenWidth, screenHeight, camera);
    return {topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y};
}

int hoveredPlacementCandidateIndex(const Simulation& simulation, TopologyMutationType type, const CameraController& camera, Vector2 mousePosition, int screenWidth, int screenHeight)
{
    if (!CheckCollisionPointRec(mousePosition, mapBounds(camera, screenWidth, screenHeight))) {
        return -1;
    }

    const PlacementCandidateGenerator generator;
    const auto candidates = generator.generate(simulation, type);
    if (candidates.empty()) {
        return -1;
    }

    int bestIndex = -1;
    float bestDistanceSquared = 1.0e12f;
    for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
        const Vec2 world = MapProjection::projectEquirectangular(candidates[static_cast<std::size_t>(i)].location);
        const Vector2 center = worldToScreen(world, screenWidth, screenHeight, camera);
        const float dx = mousePosition.x - center.x;
        const float dy = mousePosition.y - center.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestIndex = i;
        }
    }
    return bestIndex;
}
}

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

void ActionPlacementService::confirmHoveredPlacement(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const CameraController& camera, Vector2 mousePosition) const
{
    const PlacementCandidateGenerator candidateGenerator;
    const auto candidates = candidateGenerator.generate(simulation, uiState.activeMutation);
    const int index = hoveredPlacementCandidateIndex(simulation, uiState.activeMutation, camera, mousePosition, GetScreenWidth(), GetScreenHeight());
    if (index < 0 || index >= static_cast<int>(candidates.size())) {
        uiState.latestFeedback = "Hover a map region and click to place the selected node.";
        return;
    }
    confirmPlacement(simulation, scenarioManager, uiState, candidates[static_cast<std::size_t>(index)]);
}

}
