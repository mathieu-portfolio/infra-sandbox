#include "simulation/core/Simulation.hpp"

#include "simulation/systems/SimulationHealthSystem.hpp"
#include "simulation/systems/SimulationPressureSystem.hpp"
#include "simulation/systems/SimulationRequestFlowSystem.hpp"
#include "simulation/systems/SimulationScenarioBuilder.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>


Simulation::Simulation(const ScenarioDefinition& scenario, SimulationConfig config)
    : config_(config)
{
    pressureAnalysis_.setConfig(config_.pressureAnalysis);
    SimulationScenarioBuilder::buildFromScenario(*this, scenario);
}

void Simulation::update(double dt)
{
    timeSystem_.update(dt);
    timeSeconds_ = timeSystem_.state().elapsedSeconds;
    runtimeSystems_.update(dt);
    SimulationRequestFlowSystem::expireCacheEntries(*this);
    SimulationRequestFlowSystem::generateClientRequests(*this, dt);
    SimulationRequestFlowSystem::updateRetryWaits(*this);
    SimulationRequestFlowSystem::updateLinks(*this, dt);
    SimulationRequestFlowSystem::updateProcessors(*this, dt);
    SimulationHealthSystem::updatePropagatedPressure(*this, dt);
    SimulationHealthSystem::updateNodeHealth(*this, dt);
    SimulationHealthSystem::updateMetricsNodeStates(*this);
    SimulationPressureSystem::updatePressureState(*this, dt);

    metrics_.setSimulationSpeed(simulationSpeed_);
    metrics_.setRuntimeSystemCounts(runtimeSystems_.enabledCount(), static_cast<int>(runtimeSystems_.states().size()));
    metrics_.setComplexity(complexityScore_, recommendedComplexityThreshold_);
    metrics_.setPressureState(pressureState_);
    std::vector<PressureContextSignal> activeSignals = scenarioPressureSignals_;
    activeSignals.insert(activeSignals.end(), eventPressureSignals_.begin(), eventPressureSignals_.end());
    metrics_.setActivePressureSignals(std::move(activeSignals));
    metrics_.update(dt);
    pressureAnalysis_.update(timeSeconds_, dt, graph_, metrics_.snapshot());
    SimulationRequestFlowSystem::pruneOldRequests(*this);
}
