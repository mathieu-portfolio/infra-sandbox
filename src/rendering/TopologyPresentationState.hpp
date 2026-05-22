#pragma once

#include "simulation/core/Simulation.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace rendering {

struct NodePresentationState {
    float traffic = 0.0f;
    float computePressure = 0.0f;
    float memoryPressure = 0.0f;
    float storagePressure = 0.0f;
    float networkPressure = 0.0f;
    float persistenceHealth = 1.0f;
    float reliabilityRisk = 0.0f;
    float geoPresence = 0.0f;
    float queuePressure = 0.0f;
};

class TopologyPresentationState {
public:
    void update(float deltaSeconds, const Simulation& simulation)
    {
        const float dt = std::max(0.0f, deltaSeconds);
        std::unordered_set<int> activeNodeIds;
        activeNodeIds.reserve(simulation.graph().nodes().size());

        for (const auto& node : simulation.graph().nodes()) {
            activeNodeIds.insert(node.id);
            const NodePresentationState target = targetForNode(node);
            auto& visual = nodes_[node.id];
            visual.traffic = approach(visual.traffic, target.traffic, dt, 6.5f);
            visual.computePressure = approach(visual.computePressure, target.computePressure, dt, 7.5f);
            visual.memoryPressure = approach(visual.memoryPressure, target.memoryPressure, dt, 7.5f);
            visual.storagePressure = approach(visual.storagePressure, target.storagePressure, dt, 7.5f);
            visual.networkPressure = approach(visual.networkPressure, target.networkPressure, dt, 7.5f);
            visual.persistenceHealth = approach(visual.persistenceHealth, target.persistenceHealth, dt, 5.0f);
            visual.reliabilityRisk = approach(visual.reliabilityRisk, target.reliabilityRisk, dt, 8.0f);
            visual.geoPresence = approach(visual.geoPresence, target.geoPresence, dt, 5.0f);
            visual.queuePressure = approach(visual.queuePressure, target.queuePressure, dt, 8.0f);
        }

        for (auto it = nodes_.begin(); it != nodes_.end();) {
            if (activeNodeIds.find(it->first) == activeNodeIds.end()) {
                it = nodes_.erase(it);
            } else {
                ++it;
            }
        }
    }

    [[nodiscard]] const NodePresentationState& node(int nodeId) const
    {
        const auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return fallback_;
        }
        return it->second;
    }

private:
    static float approach(float current, float target, float deltaSeconds, float speed)
    {
        const float factor = std::clamp(deltaSeconds * speed, 0.0f, 1.0f);
        return current + (target - current) * factor;
    }

    static float clamp01(double value)
    {
        return static_cast<float>(std::clamp(value, 0.0, 1.0));
    }

    static NodePresentationState targetForNode(const Node& node)
    {
        NodePresentationState target{};
        target.traffic = clamp01(node.currentUtilization + node.queuePressure * 0.5 + node.networkPressure * 0.5);
        target.computePressure = clamp01(node.computePressure);
        target.memoryPressure = clamp01(node.memoryPressure);
        target.storagePressure = clamp01(node.storagePressure);
        target.networkPressure = clamp01(node.networkPressure);
        target.persistenceHealth = clamp01(1.0 - node.storagePressure);
        target.reliabilityRisk = clamp01(1.0 - node.reliabilityScore + node.propagatedInstability * 0.65 + node.timeoutPressure * 0.35);
        target.geoPresence = node.hasGeoLocation ? 1.0f : 0.0f;
        target.queuePressure = clamp01(node.queuePressure);
        return target;
    }

    std::unordered_map<int, NodePresentationState> nodes_;
    NodePresentationState fallback_{};
};

} // namespace rendering
