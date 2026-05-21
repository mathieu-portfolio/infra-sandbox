#include "simulation/core/Simulation.hpp"

#include <algorithm>
#include <cmath>

namespace {
double clamp01(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

double approach(double current, double target, double smoothing)
{
    return current + (target - current) * std::clamp(smoothing, 0.0, 1.0);
}
}

void Simulation::updatePressureState(double dt)
{
    const double smoothing = 1.0 - std::exp(-std::max(0.0, dt) / 3.0);

    double totalDemand = 0.0;
    double processorCapacity = 0.0;
    double processorUtilization = 0.0;
    int processorCount = 0;
    int apiQueueDepth = 0;
    int databaseQueueDepth = 0;
    double linkLoad = 0.0;
    double linkLatency = 0.0;
    double linkBurst = 0.0;
    int linkCount = 0;

    for (const auto& node : graph_.nodes()) {
        if (NodeRegistry::generatesRequests(node.type)) {
            totalDemand += node.requestRatePerSecond * scenarioTrafficMultiplier_ * localizedTrafficMultiplierFor(node);
        }
        if (node.isProcessor()) {
            processorCapacity += std::max(0.1, node.processingCapacityPerSecond);
            processorUtilization += node.currentUtilization;
            ++processorCount;
            if (node.type == NodeType::ApiService) {
                apiQueueDepth += static_cast<int>(node.queue.size());
            } else if (node.type == NodeType::Database || node.type == NodeType::ReadReplica) {
                databaseQueueDepth += static_cast<int>(node.queue.size());
            }
        }
    }

    for (const auto& link : graph_.links()) {
        if (!link.enabled) {
            continue;
        }
        const double safeBandwidth = std::max(1.0, link.bandwidthPerSecond);
        linkLoad += std::clamp(static_cast<double>(link.inFlightRequests.size()) / safeBandwidth, 0.0, 1.0);
        linkLatency += std::clamp(link.baseLatencySeconds * scenarioLatencyMultiplier_ / std::max(0.25, scenario_.requestTimeoutSeconds), 0.0, 1.0);
        linkBurst = std::max(linkBurst, std::clamp(static_cast<double>(link.inFlightRequests.size()) / (safeBandwidth * 2.0), 0.0, 1.0));
        ++linkCount;
    }

    const double loadRatio = processorCapacity > 0.0 ? totalDemand / processorCapacity : totalDemand;
    const double averageUtilization = processorCount > 0 ? processorUtilization / static_cast<double>(processorCount) : 0.0;
    const double queuePressure = std::clamp(static_cast<double>(apiQueueDepth + databaseQueueDepth) / std::max(1.0, totalDemand * 2.0), 0.0, 1.0);
    const double averageLinkLoad = linkCount > 0 ? linkLoad / static_cast<double>(linkCount) : 0.0;
    const double averageLinkLatency = linkCount > 0 ? linkLatency / static_cast<double>(linkCount) : 0.0;
    const double burstMultiplier = burstModeEnabled_ ? 1.0 : 0.0;

    pressureState_.backend.requestLoad = approach(pressureState_.backend.requestLoad, std::clamp(loadRatio / 1.8, 0.0, 1.0), smoothing);
    pressureState_.backend.queuePressure = approach(pressureState_.backend.queuePressure, queuePressure, smoothing);
    pressureState_.backend.computeIntensity = approach(pressureState_.backend.computeIntensity, std::clamp(averageUtilization * 0.75 + loadRatio * 0.16, 0.0, 1.0), smoothing);
    pressureState_.backend.serviceFragmentation = approach(
        pressureState_.backend.serviceFragmentation,
        std::clamp(0.10 + complexityScore_ / std::max(1.0, recommendedComplexityThreshold_) * 0.55 + static_cast<double>(processorCount) * 0.025, 0.0, 1.0),
        smoothing * 0.35);

    pressureState_.network.bandwidthPressure = approach(pressureState_.network.bandwidthPressure, std::clamp(averageLinkLoad * 0.72 + loadRatio * 0.12, 0.0, 1.0), smoothing);
    pressureState_.network.latencySensitivity = approach(pressureState_.network.latencySensitivity, std::clamp(averageLinkLatency + scenarioLatencyMultiplier_ * 0.04, 0.0, 1.0), smoothing);
    pressureState_.network.trafficBurstiness = approach(pressureState_.network.trafficBurstiness, std::clamp(linkBurst * 0.60 + burstMultiplier * 0.28 + queuePressure * 0.18, 0.0, 1.0), smoothing);

    pressureState_.frontend.assetWeight = approach(pressureState_.frontend.assetWeight, std::clamp(0.24 + pressureState_.network.bandwidthPressure * 0.18 + pressureState_.backend.serviceFragmentation * 0.06, 0.0, 1.0), smoothing * 0.45);
    pressureState_.frontend.renderComplexity = approach(pressureState_.frontend.renderComplexity, std::clamp(0.28 + pressureState_.backend.serviceFragmentation * 0.16 + pressureState_.backend.queuePressure * 0.10, 0.0, 1.0), smoothing * 0.45);
    pressureState_.frontend.cacheEfficiency = approach(pressureState_.frontend.cacheEfficiency, cacheEnabled_ ? 0.76 : 0.45, smoothing * 0.50);
    pressureState_.frontend.realtimeIntensity = approach(pressureState_.frontend.realtimeIntensity, std::clamp(0.08 + pressureState_.network.trafficBurstiness * 0.32 + pressureState_.backend.queuePressure * 0.14, 0.0, 1.0), smoothing);
    pressureState_.frontend.sessionPersistence = approach(pressureState_.frontend.sessionPersistence, std::clamp(0.58 + pressureState_.frontend.cacheEfficiency * 0.18 - pressureState_.backend.serviceFragmentation * 0.10, 0.0, 1.0), smoothing * 0.45);
    pressureState_.frontend.mobileCompatibility = approach(pressureState_.frontend.mobileCompatibility, std::clamp(0.80 - pressureState_.frontend.assetWeight * 0.16 - pressureState_.frontend.renderComplexity * 0.10, 0.0, 1.0), smoothing * 0.35);
}

void Simulation::nudgePressureState(const PressureState& delta)
{
    pressureState_.frontend.assetWeight = clamp01(pressureState_.frontend.assetWeight + delta.frontend.assetWeight);
    pressureState_.frontend.renderComplexity = clamp01(pressureState_.frontend.renderComplexity + delta.frontend.renderComplexity);
    pressureState_.frontend.cacheEfficiency = clamp01(pressureState_.frontend.cacheEfficiency + delta.frontend.cacheEfficiency);
    pressureState_.frontend.realtimeIntensity = clamp01(pressureState_.frontend.realtimeIntensity + delta.frontend.realtimeIntensity);
    pressureState_.frontend.sessionPersistence = clamp01(pressureState_.frontend.sessionPersistence + delta.frontend.sessionPersistence);
    pressureState_.frontend.mobileCompatibility = clamp01(pressureState_.frontend.mobileCompatibility + delta.frontend.mobileCompatibility);
    pressureState_.backend.requestLoad = clamp01(pressureState_.backend.requestLoad + delta.backend.requestLoad);
    pressureState_.backend.queuePressure = clamp01(pressureState_.backend.queuePressure + delta.backend.queuePressure);
    pressureState_.backend.computeIntensity = clamp01(pressureState_.backend.computeIntensity + delta.backend.computeIntensity);
    pressureState_.backend.serviceFragmentation = clamp01(pressureState_.backend.serviceFragmentation + delta.backend.serviceFragmentation);
    pressureState_.network.bandwidthPressure = clamp01(pressureState_.network.bandwidthPressure + delta.network.bandwidthPressure);
    pressureState_.network.latencySensitivity = clamp01(pressureState_.network.latencySensitivity + delta.network.latencySensitivity);
    pressureState_.network.trafficBurstiness = clamp01(pressureState_.network.trafficBurstiness + delta.network.trafficBurstiness);
    metrics_.setPressureState(pressureState_);
}

void Simulation::applyPressureEffect(const PressureState& effect)
{
    nudgePressureState(effect);
}
