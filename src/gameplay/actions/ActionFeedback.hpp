#pragma once

#include "simulation/core/Mechanics.hpp"
#include "simulation/topology/TopologyMutation.hpp"
#include "ui/core/UiTypes.hpp"

#include <string>

class Simulation;

namespace gameplay::actions {

std::string feedbackForMechanic(MechanicType mechanic, const std::string& fallbackActionName);
std::string feedbackForTopologyMutation(TopologyMutationType mutationType, const std::string& fallbackTarget);
void recordActionFeedback(UiState& uiState, const Simulation& simulation, std::string actionName, std::string target, std::string message);

}
