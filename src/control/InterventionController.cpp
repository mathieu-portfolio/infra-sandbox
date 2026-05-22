#include "control/InterventionController.hpp"

#include "gameplay/actions/ActionFeedback.hpp"
#include "gameplay/actions/ActionPlacementService.hpp"
#include "gameplay/actions/ActionQueue.hpp"
#include "gameplay/actions/ActionRules.hpp"
#include "ui/actions/ActionPanelModel.hpp"
#include "ui/actions/EventOverlay.hpp"
#include "ui/actions/WorldActionOverlay.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/viewmodels/UiFrameView.hpp"
#include "rendering/RenderPrimitives.hpp"

#include <algorithm>

namespace {
Rectangle mapBounds(const CameraController& camera, int screenWidth, int screenHeight)
{
    const Vector2 topLeft = worldToScreen({-MapProjection::worldWidth * 0.5f, -MapProjection::worldHeight * 0.5f}, screenWidth, screenHeight, camera);
    const Vector2 bottomRight = worldToScreen({MapProjection::worldWidth * 0.5f, MapProjection::worldHeight * 0.5f}, screenWidth, screenHeight, camera);
    return {topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y};
}

bool canUseMapPlacementClick(const UiState& uiState, Vector2 mousePosition, const CameraController& camera, int screenWidth, int screenHeight)
{
    if (uiState.dockLayout.activeHandle != DockResizeHandle::None || uiState.dockLayout.hoveredHandle != DockResizeHandle::None) {
        return false;
    }
    if (!uiState.placementActive || uiState.gameplayPhase != GameplayPhase::Planning || uiState.eventPopupMode != EventPopupMode::None) {
        return false;
    }
    if (uiState.worldActionDraftVisible || pointInUiPanel(mousePosition, computeUiLayout(screenWidth, screenHeight, uiState.dockLayout))) {
        return false;
    }
    return CheckCollisionPointRec(mousePosition, mapBounds(camera, screenWidth, screenHeight));
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

void confirmHoveredPlacement(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const CameraController& camera, Vector2 mousePosition)
{
    const PlacementCandidateGenerator candidateGenerator;
    const auto candidates = candidateGenerator.generate(simulation, uiState.activeMutation);
    const int index = hoveredPlacementCandidateIndex(simulation, uiState.activeMutation, camera, mousePosition, GetScreenWidth(), GetScreenHeight());
    if (index < 0 || index >= static_cast<int>(candidates.size())) {
        uiState.latestFeedback = "Hover a map region and click to place the selected node.";
        return;
    }

    const gameplay::actions::ActionPlacementService placementService;
    placementService.confirmPlacement(simulation, scenarioManager, uiState, candidates[static_cast<std::size_t>(index)]);
}

}

void InterventionController::handleActions(std::span<const InputEvent> events, Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const CameraController& camera)
{
    const gameplay::actions::ActionQueue actionQueue;
    const gameplay::actions::ActionPlacementService placementService;

    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        if (event.action == InputAction::Select) {
            if (canUseMapPlacementClick(uiState, event.mousePosition, camera, GetScreenWidth(), GetScreenHeight())) {
                confirmHoveredPlacement(simulation, scenarioManager, uiState, camera, event.mousePosition);
                uiState.suppressMapSelectionOnce = true;
            } else {
                handleActionPanelClick(event, simulation, scenarioManager, uiState);
            }
            continue;
        }

        switch (event.action) {
        case InputAction::ScaleUp:
            actionQueue.queueMechanic(simulation, uiState, {MechanicType::ScaleUp, -1, 1.5}, "Scale Up", "API service");
            break;
        case InputAction::ScaleOut:
            actionQueue.queueMechanic(simulation, uiState, {MechanicType::ScaleOut}, "Scale Out", "API service");
            break;
        case InputAction::AddCache:
            placementService.startPlacement(simulation, uiState, TopologyMutationType::AddCache);
            break;
        case InputAction::AddReadReplica:
            placementService.startPlacement(simulation, uiState, TopologyMutationType::AddReadReplica);
            break;
        case InputAction::AddQueue:
            placementService.startPlacement(simulation, uiState, TopologyMutationType::AddQueue);
            break;
        case InputAction::AddRegionalCache:
            placementService.startPlacement(simulation, uiState, TopologyMutationType::AddRegionalCache);
            break;
        case InputAction::NextPlacementCandidate:
            placementService.moveCandidate(simulation, uiState, 1);
            break;
        case InputAction::PreviousPlacementCandidate:
            placementService.moveCandidate(simulation, uiState, -1);
            break;
        case InputAction::ConfirmPlacement:
            uiState.latestFeedback = "Hover a map region and click to place the selected node.";
            break;
        case InputAction::CancelPlacement:
            uiState.placementActive = false;
            uiState.activeActionId.clear();
            break;
        case InputAction::ToggleCache:
            actionQueue.queueMechanic(simulation, uiState, {MechanicType::EnableCache}, "Toggle Cache", "Global cache behavior");
            break;
        case InputAction::ClearCache:
            actionQueue.queueMechanic(simulation, uiState, {MechanicType::ClearCache}, "Clear Cache", "Cache");
            break;
        case InputAction::ToggleRetries:
            actionQueue.queueMechanic(simulation, uiState, {MechanicType::ToggleRetries}, "Toggle Retries", "Retry policy");
            break;
        case InputAction::ToggleTrafficBurst:
            simulation.toggleBurstMode();
            break;
        case InputAction::ResetInterventions:
            simulation.resetProcessingCapacity();
            break;
        case InputAction::EnableTracing:
            actionQueue.queueMechanic(simulation, uiState, {MechanicType::EnableTracing}, "Enable Tracing", "Observability");
            break;
        case InputAction::ThrottleTrafficUp:
            actionQueue.queueMechanic(simulation, uiState, {MechanicType::ThrottleTraffic, -1, 1.0}, "Increase Traffic", "Demand");
            break;
        case InputAction::ThrottleTrafficDown:
            actionQueue.queueMechanic(simulation, uiState, {MechanicType::ThrottleTraffic, -1, -1.0}, "Decrease Traffic", "Demand");
            break;
        default:
            break;
        }
    }
}

void InterventionController::handleActionPanelClick(const InputEvent& event, Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState) const
{
    if (uiState.dockLayout.activeHandle != DockResizeHandle::None || uiState.dockLayout.hoveredHandle != DockResizeHandle::None) {
        uiState.suppressMapSelectionOnce = true;
        return;
    }

    const ActionPanelModel model;
    const UiFrameView view = buildUiFrameView(simulation, scenarioManager);
    const auto cards = model.buildCards(view, uiState, GetScreenWidth(), GetScreenHeight());
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    const UiLayout layout = computeUiLayout(screenWidth, screenHeight, uiState.dockLayout);
    const Rectangle sidebar = layout.rightSidebar;
    const gameplay::actions::ActionQueue actionQueue;
    const gameplay::actions::ActionPlacementService placementService;

    if (uiState.eventPopupMode != EventPopupMode::None) {
        const Rectangle eventOverlay = EventOverlay::overlayBounds(screenWidth, screenHeight);
        if (CheckCollisionPointRec(event.mousePosition, EventOverlay::acknowledgeButtonBounds(eventOverlay))) {
            const EventPopupMode acknowledgedMode = uiState.eventPopupMode;
            uiState.eventPopupMode = EventPopupMode::None;
            uiState.eventPopupEvents.clear();
            uiState.eventPanelVisible = false;
            uiState.eventPanelAcknowledged = true;
            uiState.worldActionDraftVisible = acknowledgedMode == EventPopupMode::PlanningStart && !uiState.worldActionDraft.empty();
            uiState.suppressMapSelectionOnce = true;
            uiState.latestFeedback = acknowledgedMode == EventPopupMode::PlanningStart
                ? (uiState.worldActionDraft.empty() ? "Event reviewed. Queue Node Actions for this plan." : "Event reviewed. Pick a World Action before selecting Node Actions.")
                : "Event recap reviewed. Continue analysis.";
            return;
        }
        if (!pointInUiPanel(event.mousePosition, layout)) {
            uiState.suppressMapSelectionOnce = true;
            return;
        }
    }

    if (uiState.gameplayPhase != GameplayPhase::Planning) {
        uiState.latestFeedback = "Actions are locked until the next planning phase.";
        return;
    }

    if (uiState.gameplayPhase == GameplayPhase::Planning && !uiState.worldActionDraft.empty()) {
        if (CheckCollisionPointRec(event.mousePosition, WorldActionOverlay::toggleBounds(screenWidth, screenHeight, uiState.dockLayout, uiState.worldActionDraftVisible, uiState.worldActionDraft))) {
            uiState.worldActionDraftVisible = !uiState.worldActionDraftVisible;
            uiState.suppressMapSelectionOnce = true;
            return;
        }
        if (uiState.worldActionDraftVisible) {
            const Rectangle overlay = WorldActionOverlay::overlayBounds(screenWidth, screenHeight, uiState.worldActionDraft);
            const int count = static_cast<int>(uiState.worldActionDraft.size());
            for (int i = 0; i < count; ++i) {
                if (!CheckCollisionPointRec(event.mousePosition, WorldActionOverlay::draftCardBounds(overlay, i, count))) {
                    continue;
                }
                const EngineeringCapacity selectedCapacity = gameplay::actions::addCapacityPreview(
                    scenarioManager.definition().engineeringCapacity,
                    uiState.worldActionDraft[static_cast<std::size_t>(i)].capacityBonus);
                std::string capacityReason;
                if (!gameplay::actions::validCapacityDistribution(selectedCapacity, capacityReason)) {
                    uiState.latestFeedback = capacityReason;
                    uiState.suppressMapSelectionOnce = true;
                    return;
                }
                uiState.selectedWorldActionIndex = i;
                uiState.worldActionCapacityBonus = uiState.worldActionDraft[static_cast<std::size_t>(i)].capacityBonus;
                uiState.worldActionDraftVisible = false;
                uiState.suppressMapSelectionOnce = true;
                uiState.latestFeedback = uiState.worldActionDraft[static_cast<std::size_t>(i)].name + " selected as the world action for this plan.";
                return;
            }
            uiState.suppressMapSelectionOnce = true;
            return;
        }
    }

    if (gameplay::actions::worldActionRequiredBeforeNodeActions(uiState) && CheckCollisionPointRec(event.mousePosition, sidebar)) {
        uiState.selectedActionIndex = -1;
        uiState.latestFeedback = gameplay::actions::nodeActionGateMessage(uiState);
        return;
    }

    const float buttonY = sidebar.y + sidebar.height - 54.0f;
    const Rectangle actionButton{sidebar.x + 12.0f, buttonY, sidebar.width - 24.0f, 40.0f};
    if (CheckCollisionPointRec(event.mousePosition, actionButton)) {
        if (uiState.placementActive) {
            uiState.latestFeedback = "Hover the map, then click a region to place this node.";
            return;
        }
        if (uiState.selectedActionIndex >= 0 && uiState.selectedActionIndex < static_cast<int>(cards.size())) {
            const auto& card = cards[static_cast<std::size_t>(uiState.selectedActionIndex)];
            if (card.available && card.kind == ActionCardKind::Mechanic) {
                const double amount = card.mechanic == MechanicType::ThrottleTraffic ? -1.0 : 1.5;
                actionQueue.queueMechanic(simulation, uiState, {card.mechanic, uiState.selection.nodeId, amount}, card.name, card.target, card.actionId);
            } else if (card.available && card.kind == ActionCardKind::TopologyMutation) {
                uiState.activeActionId = card.actionId;
                placementService.startPlacement(simulation, uiState, card.mutation);
            }
        }
        return;
    }

    const Rectangle preview{sidebar.x + 10.0f, buttonY - UiTheme::gap - 170.0f, sidebar.width - 20.0f, 170.0f};
    if (uiState.placementActive) {
        if (CheckCollisionPointRec(event.mousePosition, {preview.x + 14.0f, preview.y + 116.0f, 28.0f, 24.0f})) {
            placementService.moveCandidate(simulation, uiState, -1);
            return;
        }
        if (CheckCollisionPointRec(event.mousePosition, {preview.x + preview.width - 42.0f, preview.y + 116.0f, 28.0f, 24.0f})) {
            placementService.moveCandidate(simulation, uiState, 1);
            return;
        }
    }

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
            uiState.latestFeedback = card.name + " selected. Use the action button to apply.";
            break;
        case ActionCardKind::TopologyMutation:
            placementService.startPlacement(simulation, uiState, card.mutation);
            break;
        case ActionCardKind::ConfirmPreview:
            uiState.latestFeedback = "Hover the map, then click a region to place this node.";
            break;
        case ActionCardKind::CancelPreview:
            uiState.placementActive = false;
            uiState.activeActionId.clear();
            uiState.latestFeedback = "Topology preview cancelled.";
            break;
        }
        return;
    }
}
