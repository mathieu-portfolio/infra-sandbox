#include "simulation/core/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
double clamp01(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

double approach(double current, double target, double smoothing)
{
    return current + (target - current) * std::clamp(smoothing, 0.0, 1.0);
}

double contextValue(double scenario, double event)
{
    return std::clamp(scenario + event, -1.0, 1.0);
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
    double databaseUtilization = 0.0;
    int databaseCount = 0;
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
                databaseUtilization += node.currentUtilization;
                ++databaseCount;
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
    const double averageDatabaseUtilization = databaseCount > 0 ? databaseUtilization / static_cast<double>(databaseCount) : 0.0;
    const double databasePressure = std::clamp(static_cast<double>(databaseQueueDepth) / std::max(1.0, totalDemand), 0.0, 1.0);
    const double cacheMissRatio = cacheEnabled_ ? 1.0 - metrics_.snapshot().cacheHitRate : 1.0;
    const double burstMultiplier = burstModeEnabled_ ? 1.0 : 0.0;
    const PressureState context{
        .frontend = {
            .assetWeight = contextValue(scenarioPressureContext_.frontend.assetWeight, eventPressureContext_.frontend.assetWeight),
            .renderComplexity = contextValue(scenarioPressureContext_.frontend.renderComplexity, eventPressureContext_.frontend.renderComplexity),
            .cacheEfficiency = contextValue(scenarioPressureContext_.frontend.cacheEfficiency, eventPressureContext_.frontend.cacheEfficiency),
            .realtimeIntensity = contextValue(scenarioPressureContext_.frontend.realtimeIntensity, eventPressureContext_.frontend.realtimeIntensity),
            .sessionPersistence = contextValue(scenarioPressureContext_.frontend.sessionPersistence, eventPressureContext_.frontend.sessionPersistence),
            .mobileCompatibility = contextValue(scenarioPressureContext_.frontend.mobileCompatibility, eventPressureContext_.frontend.mobileCompatibility),
        },
        .backend = {
            .requestLoad = contextValue(scenarioPressureContext_.backend.requestLoad, eventPressureContext_.backend.requestLoad),
            .queuePressure = contextValue(scenarioPressureContext_.backend.queuePressure, eventPressureContext_.backend.queuePressure),
            .computeIntensity = contextValue(scenarioPressureContext_.backend.computeIntensity, eventPressureContext_.backend.computeIntensity),
            .serviceFragmentation = contextValue(scenarioPressureContext_.backend.serviceFragmentation, eventPressureContext_.backend.serviceFragmentation),
        },
        .network = {
            .bandwidthPressure = contextValue(scenarioPressureContext_.network.bandwidthPressure, eventPressureContext_.network.bandwidthPressure),
            .latencySensitivity = contextValue(scenarioPressureContext_.network.latencySensitivity, eventPressureContext_.network.latencySensitivity),
            .trafficBurstiness = contextValue(scenarioPressureContext_.network.trafficBurstiness, eventPressureContext_.network.trafficBurstiness),
        },
        .database = {
            .readPressure = contextValue(scenarioPressureContext_.database.readPressure, eventPressureContext_.database.readPressure),
            .writePressure = contextValue(scenarioPressureContext_.database.writePressure, eventPressureContext_.database.writePressure),
            .contention = contextValue(scenarioPressureContext_.database.contention, eventPressureContext_.database.contention),
            .replicationLag = contextValue(scenarioPressureContext_.database.replicationLag, eventPressureContext_.database.replicationLag),
        },
        .runtime = {
            .cpuPressure = contextValue(scenarioPressureContext_.runtime.cpuPressure, eventPressureContext_.runtime.cpuPressure),
            .memoryPressure = contextValue(scenarioPressureContext_.runtime.memoryPressure, eventPressureContext_.runtime.memoryPressure),
            .allocationOrGcPressure = contextValue(scenarioPressureContext_.runtime.allocationOrGcPressure, eventPressureContext_.runtime.allocationOrGcPressure),
            .schedulingPressure = contextValue(scenarioPressureContext_.runtime.schedulingPressure, eventPressureContext_.runtime.schedulingPressure),
        },
    };

    pressureState_.backend.requestLoad = approach(pressureState_.backend.requestLoad, std::clamp(loadRatio / 1.8 + context.backend.requestLoad, 0.0, 1.0), smoothing);
    pressureState_.backend.queuePressure = approach(pressureState_.backend.queuePressure, std::clamp(queuePressure + context.backend.queuePressure, 0.0, 1.0), smoothing);
    pressureState_.backend.computeIntensity = approach(pressureState_.backend.computeIntensity, std::clamp(averageUtilization * 0.75 + loadRatio * 0.16 + context.backend.computeIntensity, 0.0, 1.0), smoothing);
    pressureState_.backend.serviceFragmentation = approach(
        pressureState_.backend.serviceFragmentation,
        std::clamp(0.10 + complexityScore_ / std::max(1.0, recommendedComplexityThreshold_) * 0.55 + static_cast<double>(processorCount) * 0.025 + context.backend.serviceFragmentation, 0.0, 1.0),
        smoothing * 0.35);

    pressureState_.network.bandwidthPressure = approach(pressureState_.network.bandwidthPressure, std::clamp(averageLinkLoad * 0.72 + loadRatio * 0.12 + context.network.bandwidthPressure, 0.0, 1.0), smoothing);
    pressureState_.network.latencySensitivity = approach(pressureState_.network.latencySensitivity, std::clamp(averageLinkLatency + scenarioLatencyMultiplier_ * 0.04 + context.network.latencySensitivity, 0.0, 1.0), smoothing);
    pressureState_.network.trafficBurstiness = approach(pressureState_.network.trafficBurstiness, std::clamp(linkBurst * 0.60 + burstMultiplier * 0.28 + queuePressure * 0.18 + context.network.trafficBurstiness, 0.0, 1.0), smoothing);

    pressureState_.database.readPressure = approach(pressureState_.database.readPressure, std::clamp(averageDatabaseUtilization * 0.46 + databasePressure * 0.24 + cacheMissRatio * 0.12 + context.database.readPressure, 0.0, 1.0), smoothing);
    pressureState_.database.writePressure = approach(pressureState_.database.writePressure, std::clamp(totalDemand / std::max(1.0, processorCapacity) * 0.10 + pressureState_.backend.serviceFragmentation * 0.08 + context.database.writePressure, 0.0, 1.0), smoothing * 0.65);
    pressureState_.database.contention = approach(pressureState_.database.contention, std::clamp(averageDatabaseUtilization * 0.50 + databasePressure * 0.35 + pressureState_.network.trafficBurstiness * 0.08 + context.database.contention, 0.0, 1.0), smoothing);
    pressureState_.database.replicationLag = approach(pressureState_.database.replicationLag, std::clamp(pressureState_.database.writePressure * 0.35 + pressureState_.backend.serviceFragmentation * 0.22 + pressureState_.network.latencySensitivity * 0.12 + context.database.replicationLag, 0.0, 1.0), smoothing * 0.45);

    pressureState_.runtime.cpuPressure = approach(pressureState_.runtime.cpuPressure, std::clamp(averageUtilization * 0.68 + loadRatio * 0.18 + context.runtime.cpuPressure, 0.0, 1.0), smoothing);
    pressureState_.runtime.memoryPressure = approach(pressureState_.runtime.memoryPressure, std::clamp(queuePressure * 0.36 + pressureState_.frontend.sessionPersistence * 0.12 + pressureState_.database.contention * 0.12 + context.runtime.memoryPressure, 0.0, 1.0), smoothing * 0.65);
    pressureState_.runtime.allocationOrGcPressure = approach(pressureState_.runtime.allocationOrGcPressure, std::clamp(pressureState_.frontend.realtimeIntensity * 0.24 + pressureState_.network.trafficBurstiness * 0.18 + pressureState_.runtime.memoryPressure * 0.20 + context.runtime.allocationOrGcPressure, 0.0, 1.0), smoothing * 0.60);
    pressureState_.runtime.schedulingPressure = approach(pressureState_.runtime.schedulingPressure, std::clamp(queuePressure * 0.28 + pressureState_.backend.serviceFragmentation * 0.24 + pressureState_.runtime.cpuPressure * 0.18 + context.runtime.schedulingPressure, 0.0, 1.0), smoothing * 0.70);

    pressureState_.frontend.assetWeight = approach(pressureState_.frontend.assetWeight, std::clamp(0.24 + pressureState_.network.bandwidthPressure * 0.18 + pressureState_.backend.serviceFragmentation * 0.06 + context.frontend.assetWeight, 0.0, 1.0), smoothing * 0.45);
    pressureState_.frontend.renderComplexity = approach(pressureState_.frontend.renderComplexity, std::clamp(0.28 + pressureState_.backend.serviceFragmentation * 0.16 + pressureState_.backend.queuePressure * 0.10 + context.frontend.renderComplexity, 0.0, 1.0), smoothing * 0.45);
    pressureState_.frontend.cacheEfficiency = approach(pressureState_.frontend.cacheEfficiency, std::clamp((cacheEnabled_ ? 0.76 : 0.45) + context.frontend.cacheEfficiency, 0.0, 1.0), smoothing * 0.50);
    pressureState_.frontend.realtimeIntensity = approach(pressureState_.frontend.realtimeIntensity, std::clamp(0.08 + pressureState_.network.trafficBurstiness * 0.32 + pressureState_.backend.queuePressure * 0.14 + context.frontend.realtimeIntensity, 0.0, 1.0), smoothing);
    pressureState_.frontend.sessionPersistence = approach(pressureState_.frontend.sessionPersistence, std::clamp(0.58 + pressureState_.frontend.cacheEfficiency * 0.18 - pressureState_.backend.serviceFragmentation * 0.10 + context.frontend.sessionPersistence, 0.0, 1.0), smoothing * 0.45);
    pressureState_.frontend.mobileCompatibility = approach(pressureState_.frontend.mobileCompatibility, std::clamp(0.80 - pressureState_.frontend.assetWeight * 0.16 - pressureState_.frontend.renderComplexity * 0.10 + context.frontend.mobileCompatibility, 0.0, 1.0), smoothing * 0.35);
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
    pressureState_.database.readPressure = clamp01(pressureState_.database.readPressure + delta.database.readPressure);
    pressureState_.database.writePressure = clamp01(pressureState_.database.writePressure + delta.database.writePressure);
    pressureState_.database.contention = clamp01(pressureState_.database.contention + delta.database.contention);
    pressureState_.database.replicationLag = clamp01(pressureState_.database.replicationLag + delta.database.replicationLag);
    pressureState_.runtime.cpuPressure = clamp01(pressureState_.runtime.cpuPressure + delta.runtime.cpuPressure);
    pressureState_.runtime.memoryPressure = clamp01(pressureState_.runtime.memoryPressure + delta.runtime.memoryPressure);
    pressureState_.runtime.allocationOrGcPressure = clamp01(pressureState_.runtime.allocationOrGcPressure + delta.runtime.allocationOrGcPressure);
    pressureState_.runtime.schedulingPressure = clamp01(pressureState_.runtime.schedulingPressure + delta.runtime.schedulingPressure);
    metrics_.setPressureState(pressureState_);
}

void Simulation::applyPressureEffect(const PressureState& effect)
{
    nudgePressureState(effect);
}

void Simulation::setEventPressureContext(const PressureState& context, std::vector<PressureContextSignal> signals)
{
    eventPressureContext_ = context;
    eventPressureSignals_ = std::move(signals);
}
