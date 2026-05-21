#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


bool Simulation::applyTopologyMutation(const TopologyMutation& mutation)
{
    if (mutation.regionSlotUsage > 0) {
        for (const auto& node : mutation.nodesToCreate) {
            if (node.hasGeoLocation && !canUseRegionSlots(node.geoLocation.regionName, mutation.regionSlotUsage)) {
                return false;
            }
        }
    }

    std::vector<int> createdNodeIds;
    createdNodeIds.reserve(mutation.nodesToCreate.size());
    for (auto node : mutation.nodesToCreate) {
        createdNodeIds.push_back(graph_.addNode(std::move(node)));
    }
    refreshRegionSlots();

    for (const int linkId : mutation.linksToDisable) {
        if (Link* link = graph_.link(linkId)) {
            link->enabled = false;
            graph_.markTopologyChanged();
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
        const Node* source = graph_.node(link.sourceNodeId);
        const Node* target = graph_.node(link.targetNodeId);
        if (source != nullptr && target != nullptr && source->hasGeoLocation && target->hasGeoLocation) {
            const GeographicSystem geography;
            const double geographicLatency = geography.latencySeconds(source->geoLocation, target->geoLocation, link.baseLatencySeconds);
            link.geographicDistanceKm = MapProjection::greatCircleKilometers(source->geoLocation, target->geoLocation);
            link.geographicLatencyContributionSeconds = std::max(0.0, geographicLatency - link.baseLatencySeconds);
            link.baseLatencySeconds = geographicLatency;
        }
        graph_.addLink(std::move(link));
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
        nudgePressureState(delta);
        addComplexity(mutation.complexityCost);
    }
    return applied;
}

bool Simulation::canUseRegionSlots(const std::string& region, int slots) const
{
    if (slots <= 0 || region.empty()) {
        return true;
    }
    return regionSlotsUsed(region) + slots <= regionSlotLimit(region);
}

bool Simulation::hasAnyRegionCapacity(int slots) const
{
    if (slots <= 0 || regionSlotLimits_.empty()) {
        return true;
    }
    for (const auto& [region, limit] : regionSlotLimits_) {
        if (regionSlotsUsed(region) + slots <= limit) {
            return true;
        }
    }
    return false;
}

int Simulation::regionSlotsUsed(const std::string& region) const
{
    const auto it = regionSlotsUsed_.find(region);
    return it != regionSlotsUsed_.end() ? it->second : 0;
}

int Simulation::regionSlotLimit(const std::string& region) const
{
    const auto it = regionSlotLimits_.find(region);
    return it != regionSlotLimits_.end() ? it->second : 5;
}

void Simulation::refreshRegionSlots()
{
    regionSlotsUsed_.clear();
    regionSlotLimits_.clear();
    for (const auto& node : graph_.nodes()) {
        if (!node.hasGeoLocation || node.geoLocation.regionName.empty()) {
            continue;
        }
        regionSlotLimits_.try_emplace(node.geoLocation.regionName, 5);
        if (node.type != NodeType::ClientCluster) {
            ++regionSlotsUsed_[node.geoLocation.regionName];
        }
    }
}
