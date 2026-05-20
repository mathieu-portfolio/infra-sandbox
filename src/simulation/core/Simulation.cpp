#include "simulation/core/Simulation.hpp"

#include "simulation/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


Simulation::Simulation(const ScenarioDefinition& scenario, SimulationConfig config)
    : config_(config)
{
    pressureAnalysis_.setConfig(config_.pressureAnalysis);
    buildFromScenario(scenario);
}

void Simulation::update(double dt)
{
    timeSystem_.update(dt);
    timeSeconds_ = timeSystem_.state().elapsedSeconds;
    runtimeSystems_.update(dt);
    expireCacheEntries();
    generateClientRequests(dt);
    updateRetryWaits();
    updateLinks(dt);
    updateProcessors(dt);
    updatePropagatedPressure(dt);
    updateNodeHealth(dt);
    updateMetricsNodeStates();

    metrics_.setSimulationSpeed(simulationSpeed_);
    metrics_.setRuntimeSystemCounts(runtimeSystems_.enabledCount(), static_cast<int>(runtimeSystems_.states().size()));
    metrics_.setComplexity(complexityScore_, recommendedComplexityThreshold_);
    metrics_.update(dt);
    pressureAnalysis_.update(timeSeconds_, dt, graph_, metrics_.snapshot());
    pruneOldRequests();
}

