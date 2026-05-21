#include "simulation/metrics/CrossDomainInteraction.hpp"

#include <algorithm>

namespace {
double clamp01(double value)
{
    return std::clamp(value, 0.0, 1.0);
}
}

PressureState CrossDomainInteraction::apply(const PressureState& state)
{
    PressureState result = state;

    // Realtime features are experienced as immediacy, but they also create more server work,
    // network chatter, and runtime scheduling pressure.
    result.backend.requestLoad += state.frontend.realtimeIntensity * 0.10;
    result.network.bandwidthPressure += state.frontend.realtimeIntensity * 0.08;
    result.runtime.cpuPressure += state.frontend.realtimeIntensity * 0.06;
    result.runtime.schedulingPressure += state.frontend.realtimeIntensity * 0.04;

    // Heavy assets primarily stress delivery, and that delivery pressure is what players feel as latency.
    result.network.bandwidthPressure += state.frontend.assetWeight * 0.10;
    result.network.latencySensitivity += state.frontend.assetWeight * 0.04;

    // Queueing backend work leaks into the client experience as delayed interactions.
    result.frontend.renderComplexity += state.backend.queuePressure * 0.04;
    result.frontend.realtimeIntensity += state.backend.queuePressure * 0.03;

    // Database contention backs up services before it becomes visible in global reliability.
    result.backend.queuePressure += state.database.contention * 0.12;
    result.backend.computeIntensity += state.database.contention * 0.05;

    // Runtime memory pressure is modeled as pauses and scheduling friction, not heap internals.
    result.backend.queuePressure += state.runtime.memoryPressure * 0.08;
    result.runtime.schedulingPressure += state.runtime.memoryPressure * 0.06;

    // Bursty traffic makes queues uneven and raises reconnect-style frontend pressure.
    result.backend.queuePressure += state.network.trafficBurstiness * 0.08;
    result.frontend.realtimeIntensity += state.network.trafficBurstiness * 0.05;

    // Service fragmentation increases coordination cost across services and links.
    result.network.latencySensitivity += state.backend.serviceFragmentation * 0.07;
    result.network.bandwidthPressure += state.backend.serviceFragmentation * 0.04;
    result.runtime.schedulingPressure += state.backend.serviceFragmentation * 0.05;
    result.database.replicationLag += state.backend.serviceFragmentation * 0.04;

    result.frontend.assetWeight = clamp01(result.frontend.assetWeight);
    result.frontend.renderComplexity = clamp01(result.frontend.renderComplexity);
    result.frontend.cacheEfficiency = clamp01(result.frontend.cacheEfficiency);
    result.frontend.realtimeIntensity = clamp01(result.frontend.realtimeIntensity);
    result.frontend.sessionPersistence = clamp01(result.frontend.sessionPersistence);
    result.frontend.mobileCompatibility = clamp01(result.frontend.mobileCompatibility);
    result.backend.requestLoad = clamp01(result.backend.requestLoad);
    result.backend.queuePressure = clamp01(result.backend.queuePressure);
    result.backend.computeIntensity = clamp01(result.backend.computeIntensity);
    result.backend.serviceFragmentation = clamp01(result.backend.serviceFragmentation);
    result.network.bandwidthPressure = clamp01(result.network.bandwidthPressure);
    result.network.latencySensitivity = clamp01(result.network.latencySensitivity);
    result.network.trafficBurstiness = clamp01(result.network.trafficBurstiness);
    result.database.readPressure = clamp01(result.database.readPressure);
    result.database.writePressure = clamp01(result.database.writePressure);
    result.database.contention = clamp01(result.database.contention);
    result.database.replicationLag = clamp01(result.database.replicationLag);
    result.runtime.cpuPressure = clamp01(result.runtime.cpuPressure);
    result.runtime.memoryPressure = clamp01(result.runtime.memoryPressure);
    result.runtime.allocationOrGcPressure = clamp01(result.runtime.allocationOrGcPressure);
    result.runtime.schedulingPressure = clamp01(result.runtime.schedulingPressure);

    return result;
}
