#pragma once
class Simulation;
class SimulationHealthSystem {
public:
    static void updatePropagatedPressure(Simulation& simulation, double dt);
    static void updateNodeHealth(Simulation& simulation, double dt);
    static void updateMetricsNodeStates(Simulation& simulation);
};
