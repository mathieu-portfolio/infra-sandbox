#pragma once
#include "gameplay/Scenario.hpp"
class Simulation;
class SimulationScenarioBuilder {
public:
    static void buildFromScenario(Simulation& simulation, const ScenarioDefinition& scenario);
};
