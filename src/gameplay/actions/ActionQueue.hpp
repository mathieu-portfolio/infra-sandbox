#pragma once

#include "core/simulation/Mechanics.hpp"
#include "simulation/topology/TopologyMutation.hpp"
#include "ui/core/UiTypes.hpp"

#include <string>

class Simulation;

namespace gameplay::actions {

class ActionQueue {
public:
    void queueMechanic(const Simulation& simulation, UiState& uiState, const MechanicCommand& command, std::string actionName, std::string target, std::string actionId = {}) const;
    void queueTopologyMutation(const Simulation& simulation, UiState& uiState, const TopologyMutation& mutation, TopologyMutationType type, std::string actionName, std::string target, std::string preview, std::string actionId = {}) const;
};

}
