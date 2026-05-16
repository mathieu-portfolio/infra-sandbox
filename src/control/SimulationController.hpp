#pragma once

#include "input/InputAction.hpp"
#include "simulation/Simulation.hpp"

#include <span>

class SimulationController {
public:
    struct Result {
        bool resetRequested = false;
        bool stepRequested = false;
    };

    Result handleActions(std::span<const InputEvent> events, Simulation& simulation, bool& paused);
};
