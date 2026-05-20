#include "core/simulation/SimulationLayer.hpp"

#include <array>
#include <cassert>

namespace {
constexpr std::array<LayerDefinition, LayerRegistry::layerCount()> kLayerDefinitions = {
    LayerDefinition{SimulationLayer::Flow, "Flow", "Request movement, routing, queues, and traffic pressure.", true, "flow", {89, 196, 255, 255}},
    LayerDefinition{SimulationLayer::Resources, "Resources", "Capacity, utilization, bandwidth, storage, cost, and energy budgets.", true, "res", {176, 196, 222, 255}},
    LayerDefinition{SimulationLayer::Persistence, "Persistence", "Stateful storage, replication, cache state, and data pressure.", true, "data", {245, 184, 76, 255}},
    LayerDefinition{SimulationLayer::Reliability, "Reliability", "Timeouts, retries, health, failover, and degradation.", true, "rel", {235, 86, 100, 255}},
    LayerDefinition{SimulationLayer::Observability, "Observability", "Metrics, logs, traces, alerts, and debug inspection.", true, "obs", {139, 148, 158, 255}},
    LayerDefinition{SimulationLayer::Complexity, "Complexity", "Operational complexity, coupling, and cognitive load.", true, "cx", {210, 168, 255, 255}},
    LayerDefinition{SimulationLayer::Geography, "Geography", "Regions, edge zones, latency geography, and placement.", true, "geo", {126, 231, 135, 255}},
};

const LayerDefinition& fallbackDefinition()
{
    static constexpr LayerDefinition fallback{
        SimulationLayer::Flow,
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
