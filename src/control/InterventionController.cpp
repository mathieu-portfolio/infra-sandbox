#include "control/InterventionController.hpp"

#include "content/ContentRegistry.hpp"
#include "ui/actions/ActionPanelModel.hpp"
#include "ui/core/UiLayout.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace {
const content::InterventionDefinition* interventionFor(MechanicType mechanic)
{
    for (const auto& definition : content::ContentRegistry::instance().interventions()) {
        if (definition.mechanic == mechanic) {
            return &definition;
        }
    }
    return nullptr;
}

int capacityForDomain(const EngineeringCapacity& capacity, EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend:
        return capacity.frontend;
    case EngineeringDomain::Backend:
        return capacity.backend;
    case EngineeringDomain::Infrastructure:
        return capacity.infrastructure;
    case EngineeringDomain::Data:
        return capacity.data;
    case EngineeringDomain::Operations:
        return capacity.operations;
    case EngineeringDomain::Count:
        break;
    }
    return 0;
}

std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> plannedDomainUsage(const UiState& uiState)
{
    std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> usage{};
    for (const auto& planned : uiState.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            usage[static_cast<std::size_t>(cost.domain)] += cost.amount;
        }
    }
    return usage;
}

int plannedTotalUsage(const UiState& uiState)
{
    int total = 0;
    for (const auto& planned : uiState.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            total += cost.amount;
        }
    }
    return total;
}

bool canQueueEngineeringCosts(const UiState& uiState, const std::vector<EngineeringCost>& costs, std::string& reason)
{
    const auto usage = plannedDomainUsage(uiState);
    int total = plannedTotalUsage(uiState);
    for (const auto& cost : costs) {
        const int next = usage[static_cast<std::size_t>(cost.domain)] + cost.amount;
        const int cap = capacityForDomain(uiState.engineeringCapacity, cost.domain);
        if (next > cap) {
            reason = std::string("Insufficient ") + engineeringDomainName(cost.domain) + " capacity this turn.";
            return false;
        }
        total += cost.amount;
    }
    if (total > uiState.engineeringCapacity.total) {
        reason = "Shared engineering capacity is fully allocated this turn.";
        return false;
    }
    return true;
}

std::vector<EngineeringCost> engineeringCostsFor(MechanicType mechanic)
{
    if (const auto* definition = interventionFor(mechanic)) {
        return definition->engineeringCosts;
    }
    return {};
}

Rectangle worldActionToggleBounds(int screenWidth)
{
    return {static_cast<float>(screenWidth) * 0.5f - 120.0f, 68.0f, 240.0f, 34.0f};
}

Rectangle worldActionOverlayBounds(int screenWidth, int screenHeight)
{
    const float width = std::min(840.0f, static_cast<float>(screenWidth) - 96.0f);
    const float height = std::min(360.0f, static_cast<float>(screenHeight) - 160.0f);
    return {static_cast<float>(screenWidth) * 0.5f - width * 0.5f, static_cast<float>(screenHeight) * 0.5f - height * 0.5f, width, height};
}

Rectangle worldActionOverlayCardBounds(Rectangle overlay, int index, int count)
{
    constexpr float gap = 14.0f;
    const float contentX = overlay.x + 20.0f;
    const float contentWidth = overlay.width - 40.0f;
    const float width = (contentWidth - gap * static_cast<float>(std::max(0, count - 1))) / static_cast<float>(std::max(1, count));
    return {contentX + static_cast<float>(index) * (width + gap), overlay.y + 86.0f, width, overlay.height - 116.0f};
}

bool worldActionRequiredBeforeNodeActions(const UiState& uiState)
{
    return uiState.gameplayPhase == GameplayPhase::Planning && !uiState.worldActionDraft.empty() && uiState.selectedWorldActionIndex < 0;
}
}

void InterventionController::handleActions(std::span<const InputEvent> events, Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState)
{
    for (const auto& event : events) {
        if (event.phase != InputPhase::Pressed) {
            continue;
        }

        if (event.action == InputAction::Select) {
            handleActionPanelClick(event, simulation, scenarioManager, uiState);
            continue;
        }

        switch (event.action) {
        case InputAction::ScaleUp:
            queueMechanic(simulation, uiState, {MechanicType::ScaleUp, -1, 1.5}, "Scale Up", "API service");
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
            confirmPlacement(simulation, scenarioManager, uiState);
            break;
        case InputAction::CancelPlacement:
            uiState.placementActive = false;
            break;
        case InputAction::ToggleCache:
            queueMechanic(simulation, uiState, {MechanicType::EnableCache}, "Toggle Cache", "Global cache behavior");
            break;
        case InputAction::ClearCache:
            queueMechanic(simulation, uiState, {MechanicType::ClearCache}, "Clear Cache", "Cache");
            break;
        case InputAction::ToggleRetries:
            queueMechanic(simulation, uiState, {MechanicType::ToggleRetries}, "Toggle Retries", "Retry policy");
            break;
        case InputAction::ToggleTrafficBurst:
            simulation.toggleBurstMode();
            break;
        case InputAction::ResetInterventions:
            simulation.resetProcessingCapacity();
            break;
        case InputAction::EnableTracing:
            queueMechanic(simulation, uiState, {MechanicType::EnableTracing}, "Enable Tracing", "Observability");
            break;
        case InputAction::ThrottleTrafficUp:
            queueMechanic(simulation, uiState, {MechanicType::ThrottleTraffic, -1, 1.0}, "Increase Traffic", "Demand");
            break;
        case InputAction::ThrottleTrafficDown:
            queueMechanic(simulation, uiState, {MechanicType::ThrottleTraffic, -1, -1.0}, "Decrease Traffic", "Demand");
            break;
        default:
            break;
        }
    }
}

void InterventionController::startPlacement(const Simulation& simulation, UiState& uiState, TopologyMutationType type) const
{
    if (worldActionRequiredBeforeNodeActions(uiState)) {
        uiState.latestFeedback = "Pick a World Action before selecting Node Actions.";
        return;
    }
    const MechanicType mechanic = type == TopologyMutationType::AddCache ? MechanicType::AddCache
        : type == TopologyMutationType::AddReadReplica ? MechanicType::AddReadReplica
        : type == TopologyMutationType::AddQueue ? MechanicType::AddQueue
        : MechanicType::AddRegionalCache;
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

void InterventionController::confirmPlacement(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState) const
{
    if (!uiState.placementActive) {
        return;
    }
    const auto candidates = candidateGenerator_.generate(simulation, uiState.activeMutation);
    if (candidates.empty()) {
        return;
    }
    const int index = std::clamp(uiState.placementCandidateIndex, 0, static_cast<int>(candidates.size()) - 1);
    MutationPreview preview = mutationValidator_.preview(simulation, uiState.activeMutation, candidates[static_cast<std::size_t>(index)]);
    const MechanicType mechanic = uiState.activeMutation == TopologyMutationType::AddCache ? MechanicType::AddCache
        : uiState.activeMutation == TopologyMutationType::AddReadReplica ? MechanicType::AddReadReplica
        : uiState.activeMutation == TopologyMutationType::AddQueue ? MechanicType::AddQueue
        : MechanicType::AddRegionalCache;
    if (const auto* intervention = interventionFor(mechanic)) {
        preview.mutation.complexityCost = intervention->complexityCost;
        preview.mutation.regionSlotUsage = intervention->regionSlotUsage;
        if (!simulation.canUseRegionSlots(preview.mutation.placement.location.regionName, intervention->regionSlotUsage)) {
            uiState.latestFeedback = preview.mutation.placement.displayName + " has no free deployment slots for this action.";
            return;
        }
    }
    if (preview.valid) {
        const std::string target = candidates[static_cast<std::size_t>(index)].displayName;
        std::string feedback = std::string(topologyMutationName(uiState.activeMutation)) + " applied in " + target + ". Watch latency, queue depth, and utilization.";
        if (const auto* intervention = interventionFor(mechanic); intervention != nullptr) {
            if (!intervention->positiveEffects.empty()) {
                feedback = intervention->positiveEffects.front() + ".";
            }
            if (!intervention->pressureShifts.empty()) {
                feedback += " " + intervention->pressureShifts.front() + ".";
            }
        }
        queueTopologyMutation(simulation, uiState, preview.mutation, uiState.activeMutation, topologyMutationName(uiState.activeMutation), target, feedback);
        uiState.placementActive = false;
    }
    (void)scenarioManager;
}

void InterventionController::handleActionPanelClick(const InputEvent& event, Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState) const
{
    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, uiState, GetScreenWidth(), GetScreenHeight());
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    const UiLayout layout = computeUiLayout(screenWidth, screenHeight);
    const Rectangle sidebar = layout.rightSidebar;
    if (uiState.gameplayPhase == GameplayPhase::Planning && !uiState.worldActionDraft.empty()) {
        if (CheckCollisionPointRec(event.mousePosition, worldActionToggleBounds(screenWidth))) {
            uiState.worldActionDraftVisible = !uiState.worldActionDraftVisible;
            uiState.suppressMapSelectionOnce = true;
            return;
        }
        if (uiState.worldActionDraftVisible) {
            const Rectangle overlay = worldActionOverlayBounds(screenWidth, screenHeight);
            const int count = static_cast<int>(uiState.worldActionDraft.size());
            for (int i = 0; i < count; ++i) {
                if (!CheckCollisionPointRec(event.mousePosition, worldActionOverlayCardBounds(overlay, i, count))) {
                    continue;
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
    if (worldActionRequiredBeforeNodeActions(uiState) && CheckCollisionPointRec(event.mousePosition, sidebar)) {
        uiState.selectedActionIndex = -1;
        uiState.latestFeedback = "Pick a World Action before selecting Node Actions.";
        return;
    }
    const float buttonY = sidebar.y + sidebar.height - 54.0f;
    const Rectangle actionButton{sidebar.x + 12.0f, buttonY, sidebar.width - 24.0f, 40.0f};
    if (CheckCollisionPointRec(event.mousePosition, actionButton)) {
        if (uiState.placementActive) {
            confirmPlacement(simulation, scenarioManager, uiState);
            return;
        }
        if (uiState.selectedActionIndex >= 0 && uiState.selectedActionIndex < static_cast<int>(cards.size())) {
            const auto& card = cards[static_cast<std::size_t>(uiState.selectedActionIndex)];
            if (card.available && card.kind == ActionCardKind::Mechanic) {
                const double amount = card.mechanic == MechanicType::ThrottleTraffic ? -1.0 : 1.5;
                queueMechanic(simulation, uiState, {card.mechanic, uiState.selection.nodeId, amount}, card.name, card.target);
            } else if (card.available && card.kind == ActionCardKind::TopologyMutation) {
                startPlacement(simulation, uiState, card.mutation);
            }
        }
        return;
    }

    const Rectangle preview{sidebar.x + 10.0f, buttonY - UiTheme::gap - 170.0f, sidebar.width - 20.0f, 170.0f};
    if (uiState.placementActive) {
        if (CheckCollisionPointRec(event.mousePosition, {preview.x + 14.0f, preview.y + 116.0f, 28.0f, 24.0f})) {
            moveCandidate(simulation, uiState, -1);
            return;
        }
        if (CheckCollisionPointRec(event.mousePosition, {preview.x + preview.width - 42.0f, preview.y + 116.0f, 28.0f, 24.0f})) {
            moveCandidate(simulation, uiState, 1);
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
            startPlacement(simulation, uiState, card.mutation);
            break;
        case ActionCardKind::ConfirmPreview:
            confirmPlacement(simulation, scenarioManager, uiState);
            break;
        case ActionCardKind::CancelPreview:
            uiState.placementActive = false;
            uiState.latestFeedback = "Topology preview cancelled.";
            break;
        }
        return;
    }
}

void InterventionController::executeMechanic(Simulation& simulation, ScenarioManager& scenarioManager, UiState& uiState, const MechanicCommand& command, std::string actionName, std::string target) const
{
    const auto before = simulation.metrics();
    mechanicExecutor_.execute(simulation, command);
    scenarioManager.notifyActionTriggered(command.type);
    uiState.pendingVisualFeedbackEvents.push_back({
        .kind = command.type == MechanicType::ThrottleTraffic ? VisualFeedbackKind::TrafficShift : VisualFeedbackKind::ActionAcknowledged,
        .targetNodeId = command.targetId,
        .mechanic = command.type,
        .label = actionName,
    });

    std::string message = actionName + " applied.";
    if (const auto* intervention = interventionFor(command.type); intervention != nullptr && !intervention->positiveEffects.empty()) {
        message = intervention->positiveEffects.front() + ".";
        if (!intervention->pressureShifts.empty()) {
            message += " " + intervention->pressureShifts.front() + ".";
        }
    } else if (command.type == MechanicType::ScaleUp) {
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

void InterventionController::queueMechanic(const Simulation& simulation, UiState& uiState, const MechanicCommand& command, std::string actionName, std::string target) const
{
    if (worldActionRequiredBeforeNodeActions(uiState)) {
        uiState.latestFeedback = "Pick a World Action before selecting Node Actions.";
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
        .preview = "Queued for the next transition.",
        .engineeringCosts = costs,
    });
    uiState.latestFeedback = uiState.plannedInterventions.back().actionName + " queued. Validate the plan to simulate consequences.";
}

void InterventionController::queueTopologyMutation(const Simulation&, UiState& uiState, const TopologyMutation& mutation, TopologyMutationType type, std::string actionName, std::string target, std::string preview) const
{
    if (worldActionRequiredBeforeNodeActions(uiState)) {
        uiState.latestFeedback = "Pick a World Action before selecting Node Actions.";
        return;
    }
    const MechanicType mechanic = type == TopologyMutationType::AddCache ? MechanicType::AddCache
        : type == TopologyMutationType::AddReadReplica ? MechanicType::AddReadReplica
        : type == TopologyMutationType::AddQueue ? MechanicType::AddQueue
        : MechanicType::AddRegionalCache;
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
    uiState.latestFeedback = uiState.plannedInterventions.back().actionName + " queued. Validate the plan to simulate consequences.";
}
