#include "simulation/systems/SimulationModifierSystem.hpp"

#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


bool SimulationModifierSystem::eventLocationMatches(const EventLocation& location, const Node& node)
{
    switch (location.scope) {
    case EventLocationScope::Global:
        return true;
    case EventLocationScope::Region:
        return node.hasGeoLocation && node.geoLocation.regionName == location.region;
    case EventLocationScope::NodeType:
        return node.type == location.nodeType;
    }
    return false;
}

double SimulationModifierSystem::localizedTrafficMultiplierFor(const Simulation& simulation, const Node& node)
{
    double multiplier = 1.0;
    for (const auto& modifier : simulation.localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.trafficMultiplier;
        }
    }
    return multiplier;
}

double SimulationModifierSystem::localizedCapacityMultiplierFor(const Simulation& simulation, const Node& node)
{
    double multiplier = 1.0;
    for (const auto& modifier : simulation.localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.databaseCapacityMultiplier;
        }
    }
    return multiplier;
}

double SimulationModifierSystem::localizedRetryDelayMultiplierFor(const Simulation& simulation, const Node& node)
{
    double multiplier = 1.0;
    for (const auto& modifier : simulation.localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.retryDelayMultiplier;
        }
    }
    return multiplier;
}

void SimulationModifierSystem::refreshEffectiveCapacities(Simulation& simulation)
{
    for (auto& node : simulation.graph_.nodes()) {
        if (!node.isProcessor()) {
            continue;
        }
        node.processingCapacityPerSecond = std::max(
            0.1,
            node.baseProcessingCapacityPerSecond * node.mechanicCapacityMultiplier * node.eventCapacityMultiplier * SimulationModifierSystem::localizedCapacityMultiplierFor(simulation, node));
    }
}

