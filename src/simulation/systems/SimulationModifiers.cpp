#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


bool Simulation::eventLocationMatches(const EventLocation& location, const Node& node) const
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

double Simulation::localizedTrafficMultiplierFor(const Node& node) const
{
    double multiplier = 1.0;
    for (const auto& modifier : localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.trafficMultiplier;
        }
    }
    return multiplier;
}

double Simulation::localizedCapacityMultiplierFor(const Node& node) const
{
    double multiplier = 1.0;
    for (const auto& modifier : localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.databaseCapacityMultiplier;
        }
    }
    return multiplier;
}

double Simulation::localizedRetryDelayMultiplierFor(const Node& node) const
{
    double multiplier = 1.0;
    for (const auto& modifier : localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.retryDelayMultiplier;
        }
    }
    return multiplier;
}

void Simulation::refreshEffectiveCapacities()
{
    for (auto& node : graph_.nodes()) {
        if (!node.isProcessor()) {
            continue;
        }
        node.processingCapacityPerSecond = std::max(
            0.1,
            node.baseProcessingCapacityPerSecond * node.mechanicCapacityMultiplier * node.eventCapacityMultiplier * localizedCapacityMultiplierFor(node));
    }
}

