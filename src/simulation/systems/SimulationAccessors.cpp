#include "simulation/core/Simulation.hpp"

#include "simulation/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


const InfrastructureGraph& Simulation::graph() const
{
    return graph_;
}

const std::unordered_map<std::uint64_t, Request>& Simulation::requests() const
{
    return requests_;
}

const MetricsSnapshot& Simulation::metrics() const
{
    return metrics_.snapshot();
}

const PressureSnapshot& Simulation::pressure() const
{
    return pressureAnalysis_.snapshot();
}

const PressureAnalysisSystem& Simulation::pressureAnalysis() const
{
    return pressureAnalysis_;
}

double Simulation::timeSeconds() const
{
    return timeSeconds_;
}

bool Simulation::cacheEnabled() const
{
    return cacheEnabled_;
}

bool Simulation::burstModeEnabled() const
{
    return burstModeEnabled_;
}

bool Simulation::retriesEnabled() const
{
    return scenario_.retries.enabled;
}

double Simulation::simulationSpeed() const
{
    return simulationSpeed_;
}

bool Simulation::isMechanicAllowed(MechanicType mechanic) const
{
    const auto index = static_cast<std::size_t>(mechanic);
    if (index >= allowedMechanics_.size()) {
        return false;
    }
    return allowedMechanics_[index];
}

const RuntimeSystems& Simulation::runtimeSystems() const
{
    return runtimeSystems_;
}

const TimeState& Simulation::timeState() const
{
    return timeSystem_.state();
}

const SimulationConfig& Simulation::config() const
{
    return config_;
}

double Simulation::complexityScore() const
{
    return complexityScore_;
}

double Simulation::recommendedComplexityThreshold() const
{
    return recommendedComplexityThreshold_;
}

