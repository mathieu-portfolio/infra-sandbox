#pragma once

#include "simulation/SimulationConfig.hpp"

#include <array>
#include <span>
#include <string_view>

struct LayerSystemState {
    SimulationLayer layer = SimulationLayer::Topology;
    bool enabled = true;
    bool initialized = false;
    double elapsedSeconds = 0.0;
};

class LayerSystemSkeleton {
public:
    explicit LayerSystemSkeleton(SimulationLayer layer);

    void initialize(const SimulationConfig& config);
    void update(double dt);

    [[nodiscard]] const LayerSystemState& state() const;
    [[nodiscard]] std::string_view name() const;

protected:
    LayerSystemState state_{};
};

class TopologySystem : public LayerSystemSkeleton {
public:
    TopologySystem();
};

class FlowSystem : public LayerSystemSkeleton {
public:
    FlowSystem();
};

class ProcessingSystem : public LayerSystemSkeleton {
public:
    ProcessingSystem();
};

class ResourceSystem : public LayerSystemSkeleton {
public:
    ResourceSystem();
};

class PersistenceSystem : public LayerSystemSkeleton {
public:
    PersistenceSystem();
};

class CoordinationSystem : public LayerSystemSkeleton {
public:
    CoordinationSystem();
};

class ReliabilitySystem : public LayerSystemSkeleton {
public:
    ReliabilitySystem();
};

class ObservabilitySystem : public LayerSystemSkeleton {
public:
    ObservabilitySystem();
};

class ComplexitySystem : public LayerSystemSkeleton {
public:
    ComplexitySystem();
};

class GeographicSystem : public LayerSystemSkeleton {
public:
    GeographicSystem();
};

class EconomicEnergySystem : public LayerSystemSkeleton {
public:
    EconomicEnergySystem();
};

class EvolutionSystem : public LayerSystemSkeleton {
public:
    EvolutionSystem();
};

class ScenarioSystem : public LayerSystemSkeleton {
public:
    ScenarioSystem();
};

class LayerSystems {
public:
    void initialize(const SimulationConfig& config);
    void update(double dt);

    [[nodiscard]] std::span<const LayerSystemState> states() const;
    [[nodiscard]] int enabledCount() const;

private:
    void refreshStates();

    TopologySystem topology_;
    FlowSystem flow_;
    ProcessingSystem processing_;
    ResourceSystem resource_;
    PersistenceSystem persistence_;
    CoordinationSystem coordination_;
    ReliabilitySystem reliability_;
    ObservabilitySystem observability_;
    ComplexitySystem complexity_;
    GeographicSystem geographic_;
    EconomicEnergySystem economicEnergy_;
    EvolutionSystem evolution_;
    ScenarioSystem scenario_;
    std::array<LayerSystemState, LayerRegistry::layerCount()> states_{};
};
