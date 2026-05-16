#include "simulation/LayerSystems.hpp"

#include <array>

LayerSystemSkeleton::LayerSystemSkeleton(SimulationLayer layer)
{
    state_.layer = layer;
}

void LayerSystemSkeleton::initialize(const SimulationConfig& config)
{
    state_.enabled = config.isLayerEnabled(state_.layer);
    state_.initialized = true;
    state_.elapsedSeconds = 0.0;
}

void LayerSystemSkeleton::update(double dt)
{
    if (!state_.enabled) {
        return;
    }
    state_.elapsedSeconds += dt;
}

const LayerSystemState& LayerSystemSkeleton::state() const
{
    return state_;
}

std::string_view LayerSystemSkeleton::name() const
{
    return LayerRegistry::definition(state_.layer).displayName;
}

TopologySystem::TopologySystem() : LayerSystemSkeleton(SimulationLayer::Topology) {}
FlowSystem::FlowSystem() : LayerSystemSkeleton(SimulationLayer::Flow) {}
ProcessingSystem::ProcessingSystem() : LayerSystemSkeleton(SimulationLayer::Processing) {}
ResourceSystem::ResourceSystem() : LayerSystemSkeleton(SimulationLayer::Resource) {}
PersistenceSystem::PersistenceSystem() : LayerSystemSkeleton(SimulationLayer::Persistence) {}
CoordinationSystem::CoordinationSystem() : LayerSystemSkeleton(SimulationLayer::Coordination) {}
ReliabilitySystem::ReliabilitySystem() : LayerSystemSkeleton(SimulationLayer::Reliability) {}
ObservabilitySystem::ObservabilitySystem() : LayerSystemSkeleton(SimulationLayer::Observability) {}
ComplexitySystem::ComplexitySystem() : LayerSystemSkeleton(SimulationLayer::Complexity) {}
GeographicSystem::GeographicSystem() : LayerSystemSkeleton(SimulationLayer::Geographic) {}
EconomicEnergySystem::EconomicEnergySystem() : LayerSystemSkeleton(SimulationLayer::EconomicEnergy) {}
EvolutionSystem::EvolutionSystem() : LayerSystemSkeleton(SimulationLayer::Evolution) {}
ScenarioSystem::ScenarioSystem() : LayerSystemSkeleton(SimulationLayer::Scenario) {}

void LayerSystems::initialize(const SimulationConfig& config)
{
    topology_.initialize(config);
    flow_.initialize(config);
    processing_.initialize(config);
    resource_.initialize(config);
    persistence_.initialize(config);
    coordination_.initialize(config);
    reliability_.initialize(config);
    observability_.initialize(config);
    complexity_.initialize(config);
    geographic_.initialize(config);
    economicEnergy_.initialize(config);
    evolution_.initialize(config);
    scenario_.initialize(config);
    refreshStates();
}

void LayerSystems::update(double dt)
{
    topology_.update(dt);
    flow_.update(dt);
    processing_.update(dt);
    resource_.update(dt);
    persistence_.update(dt);
    coordination_.update(dt);
    reliability_.update(dt);
    observability_.update(dt);
    complexity_.update(dt);
    geographic_.update(dt);
    economicEnergy_.update(dt);
    evolution_.update(dt);
    scenario_.update(dt);
    refreshStates();
}

std::span<const LayerSystemState> LayerSystems::states() const
{
    return states_;
}

int LayerSystems::enabledCount() const
{
    int count = 0;
    for (const auto& state : states_) {
        if (state.enabled) {
            ++count;
        }
    }
    return count;
}

void LayerSystems::refreshStates()
{
    states_[static_cast<std::size_t>(SimulationLayer::Topology)] = topology_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Flow)] = flow_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Processing)] = processing_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Resource)] = resource_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Persistence)] = persistence_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Coordination)] = coordination_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Reliability)] = reliability_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Observability)] = observability_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Complexity)] = complexity_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Geographic)] = geographic_.state();
    states_[static_cast<std::size_t>(SimulationLayer::EconomicEnergy)] = economicEnergy_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Evolution)] = evolution_.state();
    states_[static_cast<std::size_t>(SimulationLayer::Scenario)] = scenario_.state();
}
