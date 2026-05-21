#include "simulation/systems/SimulationPressureSystem.hpp"
#include "simulation/systems/SimulationModifierSystem.hpp"

#include "simulation/core/Simulation.hpp"

#include "simulation/metrics/CrossDomainInteraction.hpp"

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

void SimulationPressureSystem::updatePressureState(Simulation& simulation, double dt)
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

    for (const auto& node : simulation.graph_.nodes()) {
        if (NodeRegistry::generatesRequests(node.type)) {
            totalDemand += node.requestRatePerSecond * simulation.scenarioTrafficMultiplier_ * SimulationModifierSystem::localizedTrafficMultiplierFor(simulation, node);
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

    for (const auto& link : simulation.graph_.links()) {
        if (!link.enabled) {
            continue;
        }
        const double safeBandwidth = std::max(1.0, link.bandwidthPerSecond);
        linkLoad += std::clamp(static_cast<double>(link.inFlightRequests.size()) / safeBandwidth, 0.0, 1.0);
        linkLatency += std::clamp(link.baseLatencySeconds * simulation.scenarioLatencyMultiplier_ / std::max(0.25, simulation.scenario_.requestTimeoutSeconds), 0.0, 1.0);
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
    const double cacheMissRatio = simulation.cacheEnabled_ ? 1.0 - simulation.metrics_.snapshot().cacheHitRate : 1.0;
    const double burstMultiplier = simulation.burstModeEnabled_ ? 1.0 : 0.0;
    const PressureState context{
        .frontend = {
            .assetWeight = contextValue(simulation.scenarioPressureContext_.frontend.assetWeight, simulation.eventPressureContext_.frontend.assetWeight),
            .renderComplexity = contextValue(simulation.scenarioPressureContext_.frontend.renderComplexity, simulation.eventPressureContext_.frontend.renderComplexity),
            .cacheEfficiency = contextValue(simulation.scenarioPressureContext_.frontend.cacheEfficiency, simulation.eventPressureContext_.frontend.cacheEfficiency),
            .realtimeIntensity = contextValue(simulation.scenarioPressureContext_.frontend.realtimeIntensity, simulation.eventPressureContext_.frontend.realtimeIntensity),
            .sessionPersistence = contextValue(simulation.scenarioPressureContext_.frontend.sessionPersistence, simulation.eventPressureContext_.frontend.sessionPersistence),
            .mobileCompatibility = contextValue(simulation.scenarioPressureContext_.frontend.mobileCompatibility, simulation.eventPressureContext_.frontend.mobileCompatibility),
        },
        .backend = {
            .requestLoad = contextValue(simulation.scenarioPressureContext_.backend.requestLoad, simulation.eventPressureContext_.backend.requestLoad),
            .queuePressure = contextValue(simulation.scenarioPressureContext_.backend.queuePressure, simulation.eventPressureContext_.backend.queuePressure),
            .computeIntensity = contextValue(simulation.scenarioPressureContext_.backend.computeIntensity, simulation.eventPressureContext_.backend.computeIntensity),
            .serviceFragmentation = contextValue(simulation.scenarioPressureContext_.backend.serviceFragmentation, simulation.eventPressureContext_.backend.serviceFragmentation),
        },
        .network = {
            .bandwidthPressure = contextValue(simulation.scenarioPressureContext_.network.bandwidthPressure, simulation.eventPressureContext_.network.bandwidthPressure),
            .latencySensitivity = contextValue(simulation.scenarioPressureContext_.network.latencySensitivity, simulation.eventPressureContext_.network.latencySensitivity),
            .trafficBurstiness = contextValue(simulation.scenarioPressureContext_.network.trafficBurstiness, simulation.eventPressureContext_.network.trafficBurstiness),
        },
        .database = {
            .readPressure = contextValue(simulation.scenarioPressureContext_.database.readPressure, simulation.eventPressureContext_.database.readPressure),
            .writePressure = contextValue(simulation.scenarioPressureContext_.database.writePressure, simulation.eventPressureContext_.database.writePressure),
            .contention = contextValue(simulation.scenarioPressureContext_.database.contention, simulation.eventPressureContext_.database.contention),
            .replicationLag = contextValue(simulation.scenarioPressureContext_.database.replicationLag, simulation.eventPressureContext_.database.replicationLag),
        },
        .runtime = {
            .cpuPressure = contextValue(simulation.scenarioPressureContext_.runtime.cpuPressure, simulation.eventPressureContext_.runtime.cpuPressure),
            .memoryPressure = contextValue(simulation.scenarioPressureContext_.runtime.memoryPressure, simulation.eventPressureContext_.runtime.memoryPressure),
            .allocationOrGcPressure = contextValue(simulation.scenarioPressureContext_.runtime.allocationOrGcPressure, simulation.eventPressureContext_.runtime.allocationOrGcPressure),
            .schedulingPressure = contextValue(simulation.scenarioPressureContext_.runtime.schedulingPressure, simulation.eventPressureContext_.runtime.schedulingPressure),
        },
    };

    simulation.pressureState_.backend.requestLoad = approach(simulation.pressureState_.backend.requestLoad, std::clamp(loadRatio / 1.8 + context.backend.requestLoad, 0.0, 1.0), smoothing);
    simulation.pressureState_.backend.queuePressure = approach(simulation.pressureState_.backend.queuePressure, std::clamp(queuePressure + context.backend.queuePressure, 0.0, 1.0), smoothing);
    simulation.pressureState_.backend.computeIntensity = approach(simulation.pressureState_.backend.computeIntensity, std::clamp(averageUtilization * 0.75 + loadRatio * 0.16 + context.backend.computeIntensity, 0.0, 1.0), smoothing);
    simulation.pressureState_.backend.serviceFragmentation = approach(
        simulation.pressureState_.backend.serviceFragmentation,
        std::clamp(0.10 + simulation.complexityScore_ / std::max(1.0, simulation.recommendedComplexityThreshold_) * 0.55 + static_cast<double>(processorCount) * 0.025 + context.backend.serviceFragmentation, 0.0, 1.0),
        smoothing * 0.35);

    simulation.pressureState_.network.bandwidthPressure = approach(simulation.pressureState_.network.bandwidthPressure, std::clamp(averageLinkLoad * 0.72 + loadRatio * 0.12 + context.network.bandwidthPressure, 0.0, 1.0), smoothing);
    simulation.pressureState_.network.latencySensitivity = approach(simulation.pressureState_.network.latencySensitivity, std::clamp(averageLinkLatency + simulation.scenarioLatencyMultiplier_ * 0.04 + context.network.latencySensitivity, 0.0, 1.0), smoothing);
    simulation.pressureState_.network.trafficBurstiness = approach(simulation.pressureState_.network.trafficBurstiness, std::clamp(linkBurst * 0.60 + burstMultiplier * 0.28 + queuePressure * 0.18 + context.network.trafficBurstiness, 0.0, 1.0), smoothing);

    simulation.pressureState_.database.readPressure = approach(simulation.pressureState_.database.readPressure, std::clamp(averageDatabaseUtilization * 0.46 + databasePressure * 0.24 + cacheMissRatio * 0.12 + context.database.readPressure, 0.0, 1.0), smoothing);
    simulation.pressureState_.database.writePressure = approach(simulation.pressureState_.database.writePressure, std::clamp(totalDemand / std::max(1.0, processorCapacity) * 0.10 + simulation.pressureState_.backend.serviceFragmentation * 0.08 + context.database.writePressure, 0.0, 1.0), smoothing * 0.65);
    simulation.pressureState_.database.contention = approach(simulation.pressureState_.database.contention, std::clamp(averageDatabaseUtilization * 0.50 + databasePressure * 0.35 + simulation.pressureState_.network.trafficBurstiness * 0.08 + context.database.contention, 0.0, 1.0), smoothing);
    simulation.pressureState_.database.replicationLag = approach(simulation.pressureState_.database.replicationLag, std::clamp(simulation.pressureState_.database.writePressure * 0.35 + simulation.pressureState_.backend.serviceFragmentation * 0.22 + simulation.pressureState_.network.latencySensitivity * 0.12 + context.database.replicationLag, 0.0, 1.0), smoothing * 0.45);

    simulation.pressureState_.runtime.cpuPressure = approach(simulation.pressureState_.runtime.cpuPressure, std::clamp(averageUtilization * 0.68 + loadRatio * 0.18 + context.runtime.cpuPressure, 0.0, 1.0), smoothing);
    simulation.pressureState_.runtime.memoryPressure = approach(simulation.pressureState_.runtime.memoryPressure, std::clamp(queuePressure * 0.36 + simulation.pressureState_.frontend.sessionPersistence * 0.12 + simulation.pressureState_.database.contention * 0.12 + context.runtime.memoryPressure, 0.0, 1.0), smoothing * 0.65);
    simulation.pressureState_.runtime.allocationOrGcPressure = approach(simulation.pressureState_.runtime.allocationOrGcPressure, std::clamp(simulation.pressureState_.frontend.realtimeIntensity * 0.24 + simulation.pressureState_.network.trafficBurstiness * 0.18 + simulation.pressureState_.runtime.memoryPressure * 0.20 + context.runtime.allocationOrGcPressure, 0.0, 1.0), smoothing * 0.60);
    simulation.pressureState_.runtime.schedulingPressure = approach(simulation.pressureState_.runtime.schedulingPressure, std::clamp(queuePressure * 0.28 + simulation.pressureState_.backend.serviceFragmentation * 0.24 + simulation.pressureState_.runtime.cpuPressure * 0.18 + context.runtime.schedulingPressure, 0.0, 1.0), smoothing * 0.70);

    simulation.pressureState_.frontend.assetWeight = approach(simulation.pressureState_.frontend.assetWeight, std::clamp(0.24 + simulation.pressureState_.network.bandwidthPressure * 0.18 + simulation.pressureState_.backend.serviceFragmentation * 0.06 + context.frontend.assetWeight, 0.0, 1.0), smoothing * 0.45);
    simulation.pressureState_.frontend.renderComplexity = approach(simulation.pressureState_.frontend.renderComplexity, std::clamp(0.28 + simulation.pressureState_.backend.serviceFragmentation * 0.16 + simulation.pressureState_.backend.queuePressure * 0.10 + context.frontend.renderComplexity, 0.0, 1.0), smoothing * 0.45);
    simulation.pressureState_.frontend.cacheEfficiency = approach(simulation.pressureState_.frontend.cacheEfficiency, std::clamp((simulation.cacheEnabled_ ? 0.76 : 0.45) + context.frontend.cacheEfficiency, 0.0, 1.0), smoothing * 0.50);
    simulation.pressureState_.frontend.realtimeIntensity = approach(simulation.pressureState_.frontend.realtimeIntensity, std::clamp(0.08 + simulation.pressureState_.network.trafficBurstiness * 0.32 + simulation.pressureState_.backend.queuePressure * 0.14 + context.frontend.realtimeIntensity, 0.0, 1.0), smoothing);
    simulation.pressureState_.frontend.sessionPersistence = approach(simulation.pressureState_.frontend.sessionPersistence, std::clamp(0.58 + simulation.pressureState_.frontend.cacheEfficiency * 0.18 - simulation.pressureState_.backend.serviceFragmentation * 0.10 + context.frontend.sessionPersistence, 0.0, 1.0), smoothing * 0.45);
    simulation.pressureState_.frontend.mobileCompatibility = approach(simulation.pressureState_.frontend.mobileCompatibility, std::clamp(0.80 - simulation.pressureState_.frontend.assetWeight * 0.16 - simulation.pressureState_.frontend.renderComplexity * 0.10 + context.frontend.mobileCompatibility, 0.0, 1.0), smoothing * 0.35);

    simulation.pressureState_ = CrossDomainInteraction::apply(simulation.pressureState_);
}

void SimulationPressureSystem::nudgePressureState(Simulation& simulation, const PressureState& delta)
{
    simulation.pressureState_.frontend.assetWeight = clamp01(simulation.pressureState_.frontend.assetWeight + delta.frontend.assetWeight);
    simulation.pressureState_.frontend.renderComplexity = clamp01(simulation.pressureState_.frontend.renderComplexity + delta.frontend.renderComplexity);
    simulation.pressureState_.frontend.cacheEfficiency = clamp01(simulation.pressureState_.frontend.cacheEfficiency + delta.frontend.cacheEfficiency);
    simulation.pressureState_.frontend.realtimeIntensity = clamp01(simulation.pressureState_.frontend.realtimeIntensity + delta.frontend.realtimeIntensity);
    simulation.pressureState_.frontend.sessionPersistence = clamp01(simulation.pressureState_.frontend.sessionPersistence + delta.frontend.sessionPersistence);
    simulation.pressureState_.frontend.mobileCompatibility = clamp01(simulation.pressureState_.frontend.mobileCompatibility + delta.frontend.mobileCompatibility);
    simulation.pressureState_.backend.requestLoad = clamp01(simulation.pressureState_.backend.requestLoad + delta.backend.requestLoad);
    simulation.pressureState_.backend.queuePressure = clamp01(simulation.pressureState_.backend.queuePressure + delta.backend.queuePressure);
    simulation.pressureState_.backend.computeIntensity = clamp01(simulation.pressureState_.backend.computeIntensity + delta.backend.computeIntensity);
    simulation.pressureState_.backend.serviceFragmentation = clamp01(simulation.pressureState_.backend.serviceFragmentation + delta.backend.serviceFragmentation);
    simulation.pressureState_.network.bandwidthPressure = clamp01(simulation.pressureState_.network.bandwidthPressure + delta.network.bandwidthPressure);
    simulation.pressureState_.network.latencySensitivity = clamp01(simulation.pressureState_.network.latencySensitivity + delta.network.latencySensitivity);
    simulation.pressureState_.network.trafficBurstiness = clamp01(simulation.pressureState_.network.trafficBurstiness + delta.network.trafficBurstiness);
    simulation.pressureState_.database.readPressure = clamp01(simulation.pressureState_.database.readPressure + delta.database.readPressure);
    simulation.pressureState_.database.writePressure = clamp01(simulation.pressureState_.database.writePressure + delta.database.writePressure);
    simulation.pressureState_.database.contention = clamp01(simulation.pressureState_.database.contention + delta.database.contention);
    simulation.pressureState_.database.replicationLag = clamp01(simulation.pressureState_.database.replicationLag + delta.database.replicationLag);
    simulation.pressureState_.runtime.cpuPressure = clamp01(simulation.pressureState_.runtime.cpuPressure + delta.runtime.cpuPressure);
    simulation.pressureState_.runtime.memoryPressure = clamp01(simulation.pressureState_.runtime.memoryPressure + delta.runtime.memoryPressure);
    simulation.pressureState_.runtime.allocationOrGcPressure = clamp01(simulation.pressureState_.runtime.allocationOrGcPressure + delta.runtime.allocationOrGcPressure);
    simulation.pressureState_.runtime.schedulingPressure = clamp01(simulation.pressureState_.runtime.schedulingPressure + delta.runtime.schedulingPressure);
    simulation.metrics_.setPressureState(simulation.pressureState_);
}

void SimulationPressureSystem::applyPressureEffect(Simulation& simulation, const PressureState& effect)
{
    SimulationPressureSystem::nudgePressureState(simulation, effect);
}

void SimulationPressureSystem::setEventPressureContext(Simulation& simulation, const PressureState& context, std::vector<PressureContextSignal> signals)
{
    simulation.eventPressureContext_ = context;
    simulation.eventPressureSignals_ = std::move(signals);
}
