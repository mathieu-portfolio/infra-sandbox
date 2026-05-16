#include "simulation/Mechanics.hpp"

#include "simulation/Simulation.hpp"

#include <array>
#include <cassert>

namespace {
constexpr std::array<SimulationLayer, 4> layers(
    SimulationLayer a,
    SimulationLayer b = SimulationLayer::Flow,
    SimulationLayer c = SimulationLayer::Flow,
    SimulationLayer d = SimulationLayer::Flow)
{
    return {a, b, c, d};
}

constexpr MechanicDefinition mechanic(
    MechanicType type,
    std::string_view displayName,
    std::string_view description,
    MechanicTargetType targetType,
    std::array<SimulationLayer, 4> affectedLayers,
    std::size_t affectedLayerCount,
    bool available)
{
    return {type, displayName, description, targetType, affectedLayers, affectedLayerCount, available};
}

constexpr std::array<MechanicDefinition, static_cast<std::size_t>(MechanicType::Count)> kDefinitions = {
    mechanic(MechanicType::ScaleUp, "Scale Up", "Increase capacity on a target service.", MechanicTargetType::Node, layers(SimulationLayer::Resources, SimulationLayer::Flow, SimulationLayer::Complexity), 3, true),
    mechanic(MechanicType::ScaleOut, "Scale Out", "Add parallel capacity through replicas.", MechanicTargetType::Node, layers(SimulationLayer::Resources, SimulationLayer::Flow, SimulationLayer::Complexity), 3, false),
    mechanic(MechanicType::EnableCache, "Enable Cache", "Toggle cache behavior for cacheable requests.", MechanicTargetType::Global, layers(SimulationLayer::Flow, SimulationLayer::Persistence, SimulationLayer::Resources, SimulationLayer::Complexity), 4, true),
    mechanic(MechanicType::ClearCache, "Clear Cache", "Evict current cache entries.", MechanicTargetType::Global, layers(SimulationLayer::Persistence, SimulationLayer::Flow), 2, true),
    mechanic(MechanicType::ToggleRetries, "Toggle Retries", "Enable or disable retry behavior.", MechanicTargetType::Global, layers(SimulationLayer::Reliability, SimulationLayer::Flow, SimulationLayer::Resources), 3, true),
    mechanic(MechanicType::AdjustRetryPolicy, "Adjust Retry Policy", "Tune retry delay or attempt count.", MechanicTargetType::Global, layers(SimulationLayer::Reliability, SimulationLayer::Complexity), 2, false),
    mechanic(MechanicType::AddQueue, "Add Queue", "Insert buffering between components.", MechanicTargetType::Link, layers(SimulationLayer::Flow, SimulationLayer::Reliability, SimulationLayer::Complexity), 3, false),
    mechanic(MechanicType::AddLoadBalancer, "Add Load Balancer", "Route traffic across compute capacity.", MechanicTargetType::Node, layers(SimulationLayer::Flow, SimulationLayer::Resources, SimulationLayer::Reliability, SimulationLayer::Complexity), 4, false),
    mechanic(MechanicType::AddReadReplica, "Add Read Replica", "Increase read capacity for persistence.", MechanicTargetType::Node, layers(SimulationLayer::Persistence, SimulationLayer::Resources, SimulationLayer::Flow, SimulationLayer::Complexity), 4, false),
    mechanic(MechanicType::ThrottleTraffic, "Throttle Traffic", "Adjust generated client demand.", MechanicTargetType::Global, layers(SimulationLayer::Flow, SimulationLayer::Reliability), 2, true),
    mechanic(MechanicType::SplitService, "Split Service", "Separate a service into smaller responsibilities.", MechanicTargetType::Node, layers(SimulationLayer::Complexity, SimulationLayer::Flow, SimulationLayer::Resources), 3, false),
    mechanic(MechanicType::EnableTracing, "Enable Tracing", "Increase observability for request paths.", MechanicTargetType::Global, layers(SimulationLayer::Observability, SimulationLayer::Complexity), 2, false),
};

const MechanicDefinition& fallbackDefinition()
{
    static constexpr auto fallback = MechanicDefinition{
        MechanicType::ScaleUp,
        "Unknown Mechanic",
        "Fallback mechanic definition.",
        MechanicTargetType::Global,
        layers(SimulationLayer::Flow),
        1,
        false,
    };
    return fallback;
}
}

const MechanicDefinition& MechanicRegistry::definition(MechanicType type)
{
    const auto index = static_cast<std::size_t>(type);
    if (index >= kDefinitions.size()) {
        return fallbackDefinition();
    }

    const auto& definition = kDefinitions[index];
    assert(definition.type == type);
    return definition.type == type ? definition : fallbackDefinition();
}

std::span<const MechanicDefinition> MechanicRegistry::definitions()
{
    return kDefinitions;
}

void MechanicExecutor::execute(Simulation& simulation, const MechanicCommand& command) const
{
    switch (command.type) {
    case MechanicType::ScaleUp:
        simulation.scaleApiCapacity(command.amount > 0.0 ? command.amount : 1.5);
        break;
    case MechanicType::EnableCache:
        simulation.toggleCache();
        break;
    case MechanicType::ClearCache:
        simulation.clearCache();
        break;
    case MechanicType::ToggleRetries:
        simulation.toggleRetries();
        break;
    case MechanicType::ThrottleTraffic:
        simulation.adjustClientRequestRates(command.amount);
        break;
    case MechanicType::ScaleOut:
    case MechanicType::AdjustRetryPolicy:
    case MechanicType::AddQueue:
    case MechanicType::AddLoadBalancer:
    case MechanicType::AddReadReplica:
    case MechanicType::SplitService:
    case MechanicType::EnableTracing:
    case MechanicType::Count:
        break;
    }
}
