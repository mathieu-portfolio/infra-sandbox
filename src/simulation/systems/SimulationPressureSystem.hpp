#pragma once
#include "simulation/metrics/Metrics.hpp"
#include <vector>
class Simulation;
class SimulationPressureSystem {
public:
    static void updatePressureState(Simulation& simulation, double dt);
    static void nudgePressureState(Simulation& simulation, const PressureState& delta);
    static void applyPressureEffect(Simulation& simulation, const PressureState& effect);
    static void setEventPressureContext(Simulation& simulation, const PressureState& context, std::vector<PressureContextSignal> signals);
};
