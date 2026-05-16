#pragma once

#include "simulation/SimulationLayer.hpp"

#include <array>

struct LayerConfig {
    SimulationLayer layer = SimulationLayer::Topology;
    bool enabled = true;
};

struct SimulationConfig {
    std::array<LayerConfig, LayerRegistry::layerCount()> layers{};
    double fixedStepSeconds = 1.0 / 60.0;
    bool deterministic = true;

    SimulationConfig();

    [[nodiscard]] bool isLayerEnabled(SimulationLayer layer) const;
    void setLayerEnabled(SimulationLayer layer, bool enabled);
};

struct ScenarioConfig {
    SimulationConfig simulation;
};
