#pragma once

#include "input/InputAction.hpp"
#include "simulation/core/Simulation.hpp"

#include <span>

class SimulationController {
public:
    struct Result {
        bool resetRequested = false;
    };

    Result handleActions(std::span<const InputEvent> events, Simulation& simulation, bool& paused);
};
