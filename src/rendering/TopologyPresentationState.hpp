#pragma once

#include "simulation/core/Simulation.hpp"

#include <algorithm>
#include <cmath>
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

struct LinkPresentationState {
    float load = 0.0f;
    float activity = 0.0f;
    float congestion = 0.0f;
    float flowOffset = 0.0f;
};

class TopologyPresentationState {
public:
    void update(float deltaSeconds, const Simulation& simulation)
    {
        const float dt = std::max(0.0f, deltaSeconds);
        visualTimeSeconds_ += dt;

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

        std::unordered_set<int> activeLinkIds;
        activeLinkIds.reserve(simulation.graph().links().size());
        for (const auto& link : simulation.graph().links()) {
            activeLinkIds.insert(link.id);
            const LinkPresentationState target = targetForLink(link);
            auto& visual = links_[link.id];
            visual.load = approach(visual.load, target.load, dt, 7.5f);
            visual.activity = approach(visual.activity, target.activity, dt, 8.0f);
            visual.congestion = approach(visual.congestion, target.congestion, dt, 8.0f);
            visual.flowOffset = wrap01(visual.flowOffset + dt * (0.12f + visual.activity * 0.72f));
        }

        for (auto it = links_.begin(); it != links_.end();) {
            if (activeLinkIds.find(it->first) == activeLinkIds.end()) {
                it = links_.erase(it);
            } else {
                ++it;
            }
        }
    }

    [[nodiscard]] float visualTimeSeconds() const
    {
        return visualTimeSeconds_;
    }

    [[nodiscard]] const NodePresentationState& node(int nodeId) const
    {
        const auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return fallbackNode_;
        }
        return it->second;
    }

    [[nodiscard]] const LinkPresentationState& link(int linkId) const
    {
        const auto it = links_.find(linkId);
        if (it == links_.end()) {
            return fallbackLink_;
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

    static float wrap01(float value)
    {
        value = std::fmod(value, 1.0f);
        return value < 0.0f ? value + 1.0f : value;
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

    static LinkPresentationState targetForLink(const Link& link)
    {
        LinkPresentationState target{};
        const double safeBandwidth = std::max(1.0, link.bandwidthPerSecond);
        target.load = clamp01(static_cast<double>(link.inFlightRequests.size()) / safeBandwidth);
        target.activity = clamp01(static_cast<double>(link.inFlightRequests.size()) / std::max(1.0, safeBandwidth * 0.35));
        target.congestion = clamp01(static_cast<double>(link.inFlightRequests.size()) / std::max(1.0, safeBandwidth * 0.75));
        return target;
    }

    float visualTimeSeconds_ = 0.0f;
    std::unordered_map<int, NodePresentationState> nodes_;
    std::unordered_map<int, LinkPresentationState> links_;
    NodePresentationState fallbackNode_{};
    LinkPresentationState fallbackLink_{};
};

} // namespace rendering
