#include "gameplay/actions/ActionRules.hpp"

#include "content/ContentRegistry.hpp"

#include <array>

namespace gameplay::actions {

const content::InterventionDefinition* interventionFor(MechanicType mechanic)
{
    for (const auto& definition : content::ContentRegistry::instance().interventions()) {
        if (definition.mechanic == mechanic) {
            return &definition;
        }
    }
    return nullptr;
}

MechanicType mechanicForMutation(TopologyMutationType type)
{
    switch (type) {
    case TopologyMutationType::AddCache:
        return MechanicType::AddCache;
    case TopologyMutationType::AddReadReplica:
        return MechanicType::AddReadReplica;
    case TopologyMutationType::AddQueue:
        return MechanicType::AddQueue;
    case TopologyMutationType::AddRegionalCache:
        return MechanicType::AddRegionalCache;
    }
    return MechanicType::AddCache;
}

std::vector<EngineeringCost> engineeringCostsFor(MechanicType mechanic)
{
    if (const auto* definition = interventionFor(mechanic)) {
        return definition->engineeringCosts;
    }
    return {};
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

const EngineeringCapacity& effectivePlanningCapacity(const UiState& uiState)
{
    return uiState.engineeringCapacityPreviewVisible && uiState.selectedWorldActionIndex >= 0
        ? uiState.previewEngineeringCapacity
        : uiState.engineeringCapacity;
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

EngineeringCapacity addCapacityPreview(EngineeringCapacity base, const EngineeringCapacity& bonus)
{
    base.frontend += bonus.frontend;
    base.backend += bonus.backend;
    base.infrastructure += bonus.infrastructure;
    base.data += bonus.data;
    base.operations += bonus.operations;
    base.total += bonus.total;
    return base;
}

bool validCapacityDistribution(const EngineeringCapacity& capacity, std::string& reason)
{
    if (capacity.frontend < 0 || capacity.backend < 0 || capacity.infrastructure < 0 || capacity.data < 0 || capacity.operations < 0) {
        reason = "This world action would reduce one specialty below zero capacity.";
        return false;
    }
    if (capacity.total < 0) {
        reason = "This world action would reduce the turn budget below zero.";
        return false;
    }
    return true;
}

bool canQueueEngineeringCosts(const UiState& uiState, const std::vector<EngineeringCost>& costs, std::string& reason)
{
    const auto usage = plannedDomainUsage(uiState);
    const EngineeringCapacity& capacity = effectivePlanningCapacity(uiState);
    int plannedTotal = 0;
    int addedTotal = 0;
    for (const int used : usage) {
        plannedTotal += used;
    }
    for (const auto& cost : costs) {
        addedTotal += cost.amount;
        const int next = usage[static_cast<std::size_t>(cost.domain)] + cost.amount;
        const int cap = capacityForDomain(capacity, cost.domain);
        if (next > cap) {
            reason = std::string("Insufficient ") + engineeringDomainName(cost.domain) + " capacity this turn.";
            return false;
        }
    }
    if (plannedTotal + addedTotal > capacity.total) {
        reason = "Insufficient turn budget.";
        return false;
    }
    return true;
}

bool worldActionRequiredBeforeNodeActions(const UiState& uiState)
{
    return uiState.gameplayPhase == GameplayPhase::Planning && (uiState.eventPopupMode != EventPopupMode::None || (!uiState.worldActionDraft.empty() && uiState.selectedWorldActionIndex < 0));
}

std::string nodeActionGateMessage(const UiState& uiState)
{
    return uiState.eventPopupMode != EventPopupMode::None ? "Review Events before selecting World or Node Actions." : "Pick a World Action before selecting Node Actions.";
}

}
