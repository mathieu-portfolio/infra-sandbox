#include "simulation/systems/SimulationTopologySystem.hpp"
#include "simulation/systems/SimulationTuningSystem.hpp"
#include "simulation/systems/SimulationPressureSystem.hpp"
#include "simulation/systems/SimulationModifierSystem.hpp"

#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


bool SimulationTopologySystem::applyTopologyMutation(Simulation& simulation, const TopologyMutation& mutation)
{
    if (mutation.regionSlotUsage > 0) {
        for (const auto& node : mutation.nodesToCreate) {
            if (node.hasGeoLocation && !canUseRegionSlots(simulation, node.geoLocation.regionName, mutation.regionSlotUsage)) {
                return false;
            }
        }
    }

    std::vector<int> createdNodeIds;
    createdNodeIds.reserve(mutation.nodesToCreate.size());
    for (auto node : mutation.nodesToCreate) {
        createdNodeIds.push_back(simulation.graph_.addNode(std::move(node)));
    }
    SimulationTopologySystem::refreshRegionSlots(simulation);

    for (const int linkId : mutation.linksToDisable) {
        if (Link* link = simulation.graph_.link(linkId)) {
            link->enabled = false;
            simulation.graph_.markTopologyChanged();
        }
    }

    for (auto link : mutation.linksToCreate) {
        for (const int createdId : createdNodeIds) {
            if (link.sourceNodeId == -1) {
                link.sourceNodeId = createdId;
            }
            if (link.targetNodeId == -1) {
                link.targetNodeId = createdId;
            }
        }
        const Node* source = simulation.graph_.node(link.sourceNodeId);
        const Node* target = simulation.graph_.node(link.targetNodeId);
        if (source != nullptr && target != nullptr && source->hasGeoLocation && target->hasGeoLocation) {
            const GeographicSystem geography;
            const double geographicLatency = geography.latencySeconds(source->geoLocation, target->geoLocation, link.baseLatencySeconds);
            link.geographicDistanceKm = MapProjection::greatCircleKilometers(source->geoLocation, target->geoLocation);
            link.geographicLatencyContributionSeconds = std::max(0.0, geographicLatency - link.baseLatencySeconds);
            link.baseLatencySeconds = geographicLatency;
        }
        simulation.graph_.addLink(std::move(link));
    }

    const bool applied = !createdNodeIds.empty() || !mutation.linksToDisable.empty() || !mutation.linksToCreate.empty();
    if (applied) {
        PressureState delta;
        delta.backend.serviceFragmentation = mutation.complexityCost * 0.035;
        if (mutation.type == TopologyMutationType::AddCache || mutation.type == TopologyMutationType::AddRegionalCache) {
            delta.frontend.cacheEfficiency = 0.08;
            delta.network.bandwidthPressure = -0.03;
        }
        if (mutation.type == TopologyMutationType::AddReadReplica) {
            delta.backend.queuePressure = -0.04;
            delta.network.latencySensitivity = 0.02;
        }
        if (mutation.type == TopologyMutationType::AddQueue) {
            delta.backend.queuePressure = -0.03;
            delta.network.trafficBurstiness = -0.04;
            delta.backend.serviceFragmentation = std::max(delta.backend.serviceFragmentation, 0.04);
        }
        SimulationPressureSystem::nudgePressureState(simulation, delta);
        SimulationTuningSystem::addComplexity(simulation, mutation.complexityCost);
    }
    return applied;
}

bool SimulationTopologySystem::canUseRegionSlots(const Simulation& simulation, const std::string& region, int slots)
{
    if (slots <= 0 || region.empty()) {
        return true;
    }
    return regionSlotsUsed(simulation, region) + slots <= regionSlotLimit(simulation, region);
}

bool SimulationTopologySystem::hasAnyRegionCapacity(const Simulation& simulation, int slots)
{
    if (slots <= 0 || simulation.regionSlotLimits_.empty()) {
        return true;
    }
    for (const auto& [region, limit] : simulation.regionSlotLimits_) {
        if (regionSlotsUsed(simulation, region) + slots <= limit) {
            return true;
        }
    }
    return false;
}

int SimulationTopologySystem::regionSlotsUsed(const Simulation& simulation, const std::string& region)
{
    const auto it = simulation.regionSlotsUsed_.find(region);
    return it != simulation.regionSlotsUsed_.end() ? it->second : 0;
}

int SimulationTopologySystem::regionSlotLimit(const Simulation& simulation, const std::string& region)
{
    const auto it = simulation.regionSlotLimits_.find(region);
    return it != simulation.regionSlotLimits_.end() ? it->second : 5;
}

void SimulationTopologySystem::refreshRegionSlots(Simulation& simulation)
{
    simulation.regionSlotsUsed_.clear();
    simulation.regionSlotLimits_.clear();
    for (const auto& node : simulation.graph_.nodes()) {
        if (!node.hasGeoLocation || node.geoLocation.regionName.empty()) {
            continue;
        }
        simulation.regionSlotLimits_.try_emplace(node.geoLocation.regionName, 5);
        if (node.type != NodeType::ClientCluster) {
            ++simulation.regionSlotsUsed_[node.geoLocation.regionName];
        }
    }
}
