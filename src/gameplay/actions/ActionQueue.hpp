#pragma once

#include "simulation/core/Mechanics.hpp"
#include "simulation/topology/TopologyMutation.hpp"
#include "ui/core/UiTypes.hpp"

#include <string>

class Simulation;

namespace gameplay::actions {

class ActionQueue {
public:
    void queueMechanic(const Simulation& simulation, UiState& uiState, const MechanicCommand& command, std::string actionName, std::string target) const;
    void queueTopologyMutation(const Simulation& simulation, UiState& uiState, const TopologyMutation& mutation, TopologyMutationType type, std::string actionName, std::string target, std::string preview) const;
};

}
