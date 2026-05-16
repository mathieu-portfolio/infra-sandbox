#pragma once

#include "input/InputAction.hpp"
#include "simulation/Mechanics.hpp"
#include "simulation/Simulation.hpp"

#include <span>

class InterventionController {
public:
    void handleActions(std::span<const InputEvent> events, Simulation& simulation);

private:
    MechanicExecutor mechanicExecutor_;
};
