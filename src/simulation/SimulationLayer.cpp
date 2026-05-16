#include "simulation/SimulationLayer.hpp"

#include <array>
#include <cassert>

namespace {
constexpr std::array<LayerDefinition, LayerRegistry::layerCount()> kLayerDefinitions = {
    LayerDefinition{SimulationLayer::Topology, "Topology", "Graph structure, links, placement, and connectivity.", true, "topo", {121, 192, 255, 255}},
    LayerDefinition{SimulationLayer::Flow, "Flow", "Request movement, routing, queues, and traffic pressure.", true, "flow", {89, 196, 255, 255}},
    LayerDefinition{SimulationLayer::Processing, "Processing", "Node capacity, service work, and utilization.", true, "cpu", {86, 210, 151, 255}},
    LayerDefinition{SimulationLayer::Resource, "Resource", "CPU, memory, bandwidth, storage, and resource budgets.", true, "res", {176, 196, 222, 255}},
    LayerDefinition{SimulationLayer::Persistence, "Persistence", "Stateful storage, replication, cache state, and data pressure.", true, "data", {245, 184, 76, 255}},
    LayerDefinition{SimulationLayer::Coordination, "Coordination", "Queues, event buses, service discovery, and orchestration.", true, "coord", {187, 128, 255, 255}},
    LayerDefinition{SimulationLayer::Reliability, "Reliability", "Timeouts, retries, health, failover, and degradation.", true, "rel", {235, 86, 100, 255}},
    LayerDefinition{SimulationLayer::Observability, "Observability", "Metrics, logs, traces, alerts, and debug inspection.", true, "obs", {139, 148, 158, 255}},
    LayerDefinition{SimulationLayer::Complexity, "Complexity", "Operational complexity, coupling, and cognitive load.", true, "cx", {210, 168, 255, 255}},
    LayerDefinition{SimulationLayer::Geographic, "Geographic", "Regions, edge zones, latency geography, and placement.", true, "geo", {126, 231, 135, 255}},
    LayerDefinition{SimulationLayer::EconomicEnergy, "Economic/Energy", "Cost, energy, efficiency, and sustainability pressure.", true, "cost", {255, 214, 102, 255}},
    LayerDefinition{SimulationLayer::Evolution, "Evolution", "Architecture drift, migration, versioning, and long-term change.", true, "evo", {255, 156, 102, 255}},
    LayerDefinition{SimulationLayer::Scenario, "Scenario", "Scenario rules, goals, initial state, and scripted events.", true, "scn", {170, 180, 195, 255}},
};

const LayerDefinition& fallbackDefinition()
{
    static constexpr LayerDefinition fallback{
        SimulationLayer::Topology,
        "Unknown Layer",
        "Fallback layer definition.",
        false,
        "unknown",
        {140, 150, 165, 255},
    };
    return fallback;
}
}

const LayerDefinition& LayerRegistry::definition(SimulationLayer layer)
{
    const auto index = static_cast<std::size_t>(layer);
    if (index >= kLayerDefinitions.size()) {
        return fallbackDefinition();
    }

    const auto& definition = kLayerDefinitions[index];
    assert(definition.layer == layer);
    return definition.layer == layer ? definition : fallbackDefinition();
}

std::span<const LayerDefinition> LayerRegistry::definitions()
{
    return kLayerDefinitions;
}
