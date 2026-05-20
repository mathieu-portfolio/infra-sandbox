#pragma once

#include "content/ContentRegistry.hpp"
#include "core/simulation/Mechanics.hpp"
#include "gameplay/Scenario.hpp"
#include "simulation/topology/TopologyMutation.hpp"
#include "ui/core/UiTypes.hpp"

#include <string>
#include <vector>

class Simulation;

namespace gameplay::actions {

const content::InterventionDefinition* interventionFor(MechanicType mechanic);
MechanicType mechanicForMutation(TopologyMutationType type);
std::vector<EngineeringCost> engineeringCostsFor(MechanicType mechanic);

int capacityForDomain(const EngineeringCapacity& capacity, EngineeringDomain domain);
EngineeringCapacity addCapacityPreview(EngineeringCapacity base, const EngineeringCapacity& bonus);
bool validCapacityDistribution(const EngineeringCapacity& capacity, std::string& reason);
bool canQueueEngineeringCosts(const UiState& uiState, const std::vector<EngineeringCost>& costs, std::string& reason);
bool worldActionRequiredBeforeNodeActions(const UiState& uiState);
std::string nodeActionGateMessage(const UiState& uiState);

}
