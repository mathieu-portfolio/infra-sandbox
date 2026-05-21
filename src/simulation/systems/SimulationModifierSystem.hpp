#pragma once
#include "gameplay/Scenario.hpp"
#include "simulation/topology/Node.hpp"
class Simulation;
class SimulationModifierSystem {
public:
    [[nodiscard]] static bool eventLocationMatches(const EventLocation& location, const Node& node);
    [[nodiscard]] static double localizedTrafficMultiplierFor(const Simulation& simulation, const Node& node);
    [[nodiscard]] static double localizedCapacityMultiplierFor(const Simulation& simulation, const Node& node);
    [[nodiscard]] static double localizedRetryDelayMultiplierFor(const Simulation& simulation, const Node& node);
    static void refreshEffectiveCapacities(Simulation& simulation);
};
