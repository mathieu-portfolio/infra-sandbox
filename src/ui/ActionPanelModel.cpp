#include "ui/ActionPanelModel.hpp"

#include "content/ContentRegistry.hpp"
#include "ui/UiLayout.hpp"

#include <algorithm>
#include <array>

namespace {
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

std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> plannedDomainUsage(const UiState& state)
{
    std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> usage{};
    for (const auto& planned : state.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            usage[static_cast<std::size_t>(cost.domain)] += cost.amount;
        }
    }
    return usage;
}

int plannedTotalUsage(const UiState& state)
{
    int total = 0;
    for (const auto& planned : state.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            total += cost.amount;
        }
    }
    return total;
}

bool exceedsEngineeringCapacity(const UiState& state, const std::vector<EngineeringCost>& costs, std::string& reason)
{
    const auto usage = plannedDomainUsage(state);
    int total = plannedTotalUsage(state);
    for (const auto& cost : costs) {
        const int next = usage[static_cast<std::size_t>(cost.domain)] + cost.amount;
        const int cap = capacityForDomain(state.engineeringCapacity, cost.domain);
        if (next > cap) {
            reason = std::string("Insufficient ") + engineeringDomainName(cost.domain) + " capacity this turn.";
            return true;
        }
        total += cost.amount;
    }
    if (total > state.engineeringCapacity.total) {
        reason = "Shared engineering capacity is fully allocated this turn.";
        return true;
    }
    return false;
}

bool targetsSelectedNode(const Simulation& simulation, const UiState& state, const content::InterventionDefinition& definition)
{
    const Node* node = simulation.graph().node(state.selection.nodeId);
    if (node == nullptr) {
        return false;
    }
    if (definition.targetNodeTypes.empty()) {
        return definition.mechanic == MechanicType::ToggleRetries || definition.mechanic == MechanicType::ThrottleTraffic;
    }
    return std::find(definition.targetNodeTypes.begin(), definition.targetNodeTypes.end(), node->type) != definition.targetNodeTypes.end();
}

bool pressureMatches(const Simulation& simulation, const UiState& state, const content::InterventionDefinition& definition)
{
    const NodePressure* pressure = simulation.pressureAnalysis().pressureForNode(state.selection.nodeId);
    if (pressure == nullptr || pressure->dominant == PressureCategory::None) {
        return false;
    }
    return std::find(definition.affectedPressures.begin(), definition.affectedPressures.end(), pressure->dominant) != definition.affectedPressures.end();
}

std::string selectedTarget(const Simulation& simulation, const UiState& state)
{
    if (const Node* node = simulation.graph().node(state.selection.nodeId)) {
        return node->name;
    }
    return "Global";
}

ActionCard mechanicCard(const Simulation& simulation, const UiState& state, const content::InterventionDefinition& definition)
{
    bool available = simulation.isMechanicAllowed(definition.mechanic);
    std::string unavailableReason = available ? "" : "Locked by scenario progression.";
    const bool recommended = pressureMatches(simulation, state, definition);
    std::string stateLabel = available ? (recommended ? "Suggested" : "Available") : "Locked";
    int currentScaleLevel = 0;
    int maxScaleLevel = 0;
    if (definition.mechanic == MechanicType::ScaleUp) {
        maxScaleLevel = simulation.maxScaleLevelForNode(state.selection.nodeId, definition.maxScaleLevel);
        currentScaleLevel = simulation.scaleLevelForNode(state.selection.nodeId);
        if (state.selection.nodeId >= 0) {
            const Node* node = simulation.graph().node(state.selection.nodeId);
            if (node == nullptr || node->type != NodeType::ApiService) {
                available = false;
                unavailableReason = "Invalid target: select an API service.";
                stateLabel = "Invalid target";
            }
        }
        if (available && !simulation.canScaleNode(state.selection.nodeId, definition.maxScaleLevel)) {
            available = false;
            unavailableReason = "Max scale level reached.";
            stateLabel = "Maxed";
        }
    }
    std::string capacityReason;
    if (available && exceedsEngineeringCapacity(state, definition.engineeringCosts, capacityReason)) {
        available = false;
        unavailableReason = capacityReason;
        stateLabel = "Capacity";
    }
    return {
        .kind = ActionCardKind::Mechanic,
        .mechanic = definition.mechanic,
        .name = definition.displayName,
        .description = definition.description,
        .target = selectedTarget(simulation, state),
        .helps = definition.expectedBenefits,
        .tradeOff = definition.tradeoffs,
        .positiveEffects = definition.positiveEffects,
        .negativeEffects = definition.negativeEffects,
        .pressureShifts = definition.pressureShifts,
        .categories = definition.categories,
        .usefulWhen = definition.usefulWhen,
        .affectedPressures = definition.affectedPressures,
        .engineeringCosts = definition.engineeringCosts,
        .architecturalPattern = definition.architecturalPattern,
        .technologyExample = definition.technologyExample,
        .unavailableReason = unavailableReason,
        .stateLabel = stateLabel,
        .recommended = recommended,
        .complexityCost = definition.complexityCost,
        .currentScaleLevel = currentScaleLevel,
        .maxScaleLevel = maxScaleLevel,
        .available = available,
        .requiresConfirmation = definition.requiresConfirmation,
    };
}

ActionCard topologyCard(const Simulation& simulation, const UiState& state, const content::InterventionDefinition& definition)
{
    bool available = simulation.isMechanicAllowed(definition.mechanic);
    std::string unavailableReason = available ? "" : "Locked by scenario progression.";
    const bool recommended = pressureMatches(simulation, state, definition);
    std::string stateLabel = available ? (recommended ? "Suggested" : "Available") : "Locked";
    if (available && !simulation.hasAnyRegionCapacity(definition.regionSlotUsage)) {
        available = false;
        unavailableReason = "Insufficient regional deployment capacity.";
        stateLabel = "No capacity";
    }
    std::string capacityReason;
    if (available && exceedsEngineeringCapacity(state, definition.engineeringCosts, capacityReason)) {
        available = false;
        unavailableReason = capacityReason;
        stateLabel = "Capacity";
    }
    return {
        .kind = ActionCardKind::TopologyMutation,
        .mechanic = definition.mechanic,
        .mutation = definition.mutation,
        .name = definition.displayName,
        .description = definition.description,
        .target = selectedTarget(simulation, state),
        .helps = definition.expectedBenefits,
        .tradeOff = definition.tradeoffs,
        .positiveEffects = definition.positiveEffects,
        .negativeEffects = definition.negativeEffects,
        .pressureShifts = definition.pressureShifts,
        .categories = definition.categories,
        .usefulWhen = definition.usefulWhen,
        .affectedPressures = definition.affectedPressures,
        .engineeringCosts = definition.engineeringCosts,
        .architecturalPattern = definition.architecturalPattern,
        .technologyExample = definition.technologyExample,
        .unavailableReason = unavailableReason,
        .stateLabel = stateLabel,
        .recommended = recommended,
        .complexityCost = definition.complexityCost,
        .regionSlotUsage = definition.regionSlotUsage,
        .available = available,
        .requiresConfirmation = definition.requiresConfirmation,
    };
}
}

Rectangle ActionPanelModel::panelBounds(int screenWidth, int screenHeight) const
{
    return computeUiLayout(screenWidth, screenHeight).rightSidebar;
}

std::vector<ActionCard> ActionPanelModel::buildCards(const Simulation& simulation, const UiState& state, int screenWidth, int screenHeight) const
{
    std::vector<ActionCard> cards;
    if (state.placementActive) {
        cards.push_back({
            .kind = ActionCardKind::ConfirmPreview,
            .name = "Confirm placement",
            .description = "Queue the previewed topology change.",
            .target = selectedTarget(simulation, state),
            .helps = "Adds this architecture change to the plan.",
            .tradeOff = "Consequences resolve during transition.",
            .available = true,
        });
        cards.push_back({
            .kind = ActionCardKind::CancelPreview,
            .name = "Cancel preview",
            .description = "Exit placement without changing topology.",
            .target = "Preview",
            .helps = "Keeps current architecture unchanged.",
            .tradeOff = "No effect.",
            .available = true,
        });
    } else {
        for (const auto& definition : content::ContentRegistry::instance().interventions()) {
            if (!targetsSelectedNode(simulation, state, definition)) {
                continue;
            }
            if (definition.kind == content::InterventionKind::TopologyMutation) {
                cards.push_back(topologyCard(simulation, state, definition));
            } else {
                cards.push_back(mechanicCard(simulation, state, definition));
            }
        }
        std::stable_sort(cards.begin(), cards.end(), [](const ActionCard& lhs, const ActionCard& rhs) {
            return lhs.recommended && !rhs.recommended;
        });
    }

    const Rectangle panel = panelBounds(screenWidth, screenHeight);
    const Rectangle target{panel.x + 10.0f, panel.y + 10.0f, panel.width - 20.0f, 198.0f};
    const float buttonY = panel.y + panel.height - 54.0f;
    const float previewY = buttonY - UiTheme::gap - 170.0f;
    const Rectangle actions{panel.x + 10.0f, target.y + target.height + UiTheme::gap, panel.width - 20.0f, previewY - (target.y + target.height + UiTheme::gap) - UiTheme::gap};
    float y = actions.y + 54.0f;
    const float cardHeight = 72.0f;
    const float step = 76.0f;
    for (auto& card : cards) {
        card.bounds = {panel.x + 12.0f, y, panel.width - 24.0f, cardHeight};
        y += step;
    }
    return cards;
}
