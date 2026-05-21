#include "simulation/metrics/Metrics.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
double clampPercent(double value)
{
    return std::clamp(value, 0.0, 100.0);
}

double clamp01(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

void addContribution(
    MetricsSnapshot& snapshot,
    GlobalMetricId target,
    MetricContributionDomain domain,
    const char* label,
    double amount)
{
    if (std::abs(amount) < 0.01) {
        return;
    }
    snapshot.contributions.push_back({
        .target = target,
        .domain = domain,
        .label = label,
        .amount = amount,
    });
}
}

void MetricsAggregator::update(MetricsSnapshot& snapshot)
{
    snapshot.contributions.clear();

    const double queueDepth = static_cast<double>(snapshot.apiQueueDepth + snapshot.databaseQueueDepth);
    const double busiestNodeUtilization = std::max(snapshot.apiUtilization, snapshot.databaseUtilization);
    const double trafficBacklog = std::max(0.0, snapshot.inputRatePerSecond - snapshot.processedPerSecond);
    const PressureState& state = snapshot.pressureState;

    FrontendMetrics frontend;
    frontend.renderLatency = snapshot.averageLatencySeconds
        + state.frontend.renderComplexity * 0.18
        + state.frontend.assetWeight * 0.08
        + (1.0 - state.frontend.mobileCompatibility) * 0.10;
    frontend.framePressure = clampPercent(
        busiestNodeUtilization * 55.0
        + queueDepth * 1.1
        + state.frontend.renderComplexity * 30.0
        + state.backend.computeIntensity * 10.0);
    frontend.assetBandwidth = clampPercent(
        snapshot.inputRatePerSecond * (2.0 + state.frontend.assetWeight * 5.0)
        + state.network.bandwidthPressure * 22.0);
    frontend.interactionDelay = snapshot.averageLatencySeconds
        + queueDepth * 0.008
        + state.network.latencySensitivity * 0.16
        + state.backend.queuePressure * 0.10;
    frontend.perceivedLatency = frontend.renderLatency * 0.65 + frontend.interactionDelay * 0.35;
    frontend.websocketPressure = clampPercent(
        snapshot.retryRatePerSecond * 14.0
        + trafficBacklog * 4.0
        + state.frontend.realtimeIntensity * 44.0
        + state.network.trafficBurstiness * 12.0);
    frontend.sessionWarmth = clampPercent(
        snapshot.cacheHitRate * 55.0
        + state.frontend.cacheEfficiency * 28.0
        + state.frontend.sessionPersistence * 17.0);
    frontend.sessionStalenessRisk = clampPercent(
        snapshot.timeoutRatePerSecond * 18.0
        + snapshot.retryRatePerSecond * 6.0
        + (1.0 - state.frontend.sessionPersistence) * 20.0
        + state.backend.serviceFragmentation * 12.0);
    snapshot.frontend = frontend;

    BackendMetrics backend;
    backend.requestLoad = clampPercent(state.backend.requestLoad * 100.0);
    backend.queuePressure = clampPercent(state.backend.queuePressure * 100.0);
    backend.computeIntensity = clampPercent(state.backend.computeIntensity * 100.0);
    backend.serviceFragmentation = clampPercent(state.backend.serviceFragmentation * 100.0);
    backend.reliabilityRisk = clampPercent(state.backend.queuePressure * 40.0 + state.backend.serviceFragmentation * 35.0 + snapshot.timeoutRatePerSecond * 10.0);
    snapshot.backend = backend;

    NetworkMetrics network;
    network.bandwidthPressure = clampPercent(state.network.bandwidthPressure * 100.0);
    network.latencySensitivity = clampPercent(state.network.latencySensitivity * 100.0);
    network.trafficBurstiness = clampPercent(state.network.trafficBurstiness * 100.0);
    network.deliveryPressure = clampPercent(state.network.bandwidthPressure * 42.0 + state.network.latencySensitivity * 30.0 + state.network.trafficBurstiness * 28.0);
    snapshot.network = network;

    DatabaseMetrics database;
    database.readPressure = clampPercent(state.database.readPressure * 100.0);
    database.writePressure = clampPercent(state.database.writePressure * 100.0);
    database.contention = clampPercent(state.database.contention * 100.0);
    database.replicationLag = clampPercent(state.database.replicationLag * 100.0);
    database.persistenceRisk = clampPercent(
        database.readPressure * 0.22
        + database.writePressure * 0.18
        + database.contention * 0.36
        + database.replicationLag * 0.24);
    snapshot.database = database;

    RuntimeMetrics runtime;
    runtime.cpuPressure = clampPercent(state.runtime.cpuPressure * 100.0);
    runtime.memoryPressure = clampPercent(state.runtime.memoryPressure * 100.0);
    runtime.allocationOrGcPressure = clampPercent(state.runtime.allocationOrGcPressure * 100.0);
    runtime.schedulingPressure = clampPercent(state.runtime.schedulingPressure * 100.0);
    runtime.executionRisk = clampPercent(
        runtime.cpuPressure * 0.30
        + runtime.memoryPressure * 0.24
        + runtime.allocationOrGcPressure * 0.20
        + runtime.schedulingPressure * 0.26);
    snapshot.runtime = runtime;

    const double latencyPenalty = clampPercent(frontend.perceivedLatency * 26.0);
    const double assetLatencyPenalty = state.frontend.assetWeight * 5.0;
    const double backendQueueLatencyPenalty = state.backend.queuePressure * 8.0;
    const double networkLatencyPenalty = state.network.latencySensitivity * 5.0;
    const double framePenalty = frontend.framePressure * 0.18;
    const double warmthBonus = frontend.sessionWarmth * 0.08;
    const double assetPressure = frontend.assetBandwidth * 0.22;
    const double websocketPressure = frontend.websocketPressure * 0.35;
    const double backendPressure = backend.requestLoad * 0.16 + backend.queuePressure * 0.12 + backend.computeIntensity * 0.08;
    const double networkPressure = network.deliveryPressure * 0.20;
    const double databasePressure = database.persistenceRisk * 0.18;
    const double runtimePressure = runtime.executionRisk * 0.15;
    const double frontendReliabilityRisk = frontend.sessionStalenessRisk * 0.42;
    const double backendReliabilityRisk = backend.reliabilityRisk * 0.30;
    const double databaseReliabilityRisk = database.persistenceRisk * 0.28 + database.replicationLag * 0.20;
    const double runtimeReliabilityRisk = runtime.executionRisk * 0.18;
    const double reliabilityPenalty = clampPercent(
        frontendReliabilityRisk
        + backendReliabilityRisk
        + databaseReliabilityRisk
        + runtimeReliabilityRisk
        + snapshot.timeoutRatePerSecond * 6.0);
    const double persistenceComplexity = snapshot.totalCacheLookups > 0 ? 2.0 : 0.0;
    const double runtimeComplexity = snapshot.observability.enabledSystemCount > 0
        ? static_cast<double>(snapshot.observability.enabledSystemCount) * 0.25 + runtime.allocationOrGcPressure * 0.03
        : 0.0;
    const double serviceComplexity = backend.serviceFragmentation * 0.18;
    const double databaseComplexity = database.replicationLag * 0.05 + database.contention * 0.03;
    const double complexityPressure = snapshot.complexity.recommendedThreshold > 0.0
        ? (snapshot.complexity.current / snapshot.complexity.recommendedThreshold) * 100.0
        : snapshot.complexity.current * 10.0;

    GlobalMetrics global;
    global.userExperience = clampPercent(100.0 - latencyPenalty - framePenalty + warmthBonus);
    global.infrastructurePressure = clampPercent(
        busiestNodeUtilization * 42.0
        + queueDepth * 1.2
        + assetPressure
        + websocketPressure
        + backendPressure
        + networkPressure
        + databasePressure
        + runtimePressure
        - frontend.sessionWarmth * 0.08);
    global.reliability = clampPercent(100.0 - reliabilityPenalty);
    global.complexity = clampPercent(complexityPressure + persistenceComplexity + runtimeComplexity + serviceComplexity + databaseComplexity);
    global.scalability = clampPercent(100.0 - global.infrastructurePressure * 0.45 - global.complexity * 0.2 + frontend.sessionWarmth * 0.08);
    snapshot.global = global;

    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Frontend, "Perceived latency", -latencyPenalty);
    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Frontend, "Asset weight latency", -assetLatencyPenalty);
    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Backend, "Backend queue delay", -backendQueueLatencyPenalty);
    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Network, "Network latency sensitivity", -networkLatencyPenalty);
    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Frontend, "Frame pressure", -framePenalty);
    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Frontend, "Session warmth", warmthBonus);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Frontend, "Asset bandwidth", assetPressure);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Frontend, "Websocket pressure", websocketPressure);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Frontend, "Session warmth", -frontend.sessionWarmth * 0.08);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Backend, "Backend pressure", backendPressure);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Network, "Network delivery", networkPressure);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Database, "Persistence pressure", databasePressure);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Runtime, "Runtime pressure", runtimePressure);
    addContribution(snapshot, GlobalMetricId::Reliability, MetricContributionDomain::Frontend, "Session staleness risk", -frontendReliabilityRisk);
    addContribution(snapshot, GlobalMetricId::Reliability, MetricContributionDomain::Backend, "Backend reliability risk", -backendReliabilityRisk);
    addContribution(snapshot, GlobalMetricId::Reliability, MetricContributionDomain::Database, "Database reliability risk", -databaseReliabilityRisk);
    addContribution(snapshot, GlobalMetricId::Reliability, MetricContributionDomain::Runtime, "Runtime execution risk", -runtimeReliabilityRisk);
    addContribution(snapshot, GlobalMetricId::Complexity, MetricContributionDomain::Database, "Persistence/session systems", persistenceComplexity);
    addContribution(snapshot, GlobalMetricId::Complexity, MetricContributionDomain::Database, "Database coordination", databaseComplexity);
    addContribution(snapshot, GlobalMetricId::Complexity, MetricContributionDomain::Runtime, "Runtime systems", runtimeComplexity);
    addContribution(snapshot, GlobalMetricId::Complexity, MetricContributionDomain::Backend, "Service fragmentation", serviceComplexity);
}

void Metrics::reset()
{
    snapshot_ = {};
    windowElapsed_ = 0.0;
    generatedInWindow_ = 0;
    processedInWindow_ = 0;
    timedOutInWindow_ = 0;
    retriesInWindow_ = 0;
    cacheHitsInWindow_ = 0;
    cacheLookupsInWindow_ = 0;
    latencySumInWindow_ = 0.0;
    latencySamplesInWindow_ = 0;
}

void Metrics::recordGenerated()
{
    ++generatedInWindow_;
    ++snapshot_.totalGenerated;
}

void Metrics::recordProcessed(double latencySeconds)
{
    ++processedInWindow_;
    ++latencySamplesInWindow_;
    latencySumInWindow_ += latencySeconds;
    ++snapshot_.totalProcessed;
}

void Metrics::recordTimedOut(double latencySeconds)
{
    ++timedOutInWindow_;
    ++latencySamplesInWindow_;
    latencySumInWindow_ += latencySeconds;
    ++snapshot_.totalTimedOut;
}

void Metrics::recordRetry()
{
    ++retriesInWindow_;
    ++snapshot_.totalRetries;
}

void Metrics::recordCacheLookup(bool hit)
{
    ++cacheLookupsInWindow_;
    ++snapshot_.totalCacheLookups;
    if (hit) {
        ++cacheHitsInWindow_;
        ++snapshot_.totalCacheHits;
    }
}

void Metrics::setNodeStates(int apiQueueDepth, double apiUtilization, int databaseQueueDepth, double databaseUtilization)
{
    snapshot_.apiQueueDepth = apiQueueDepth;
    snapshot_.apiUtilization = apiUtilization;
    snapshot_.databaseQueueDepth = databaseQueueDepth;
    snapshot_.databaseUtilization = databaseUtilization;
    snapshot_.resource.apiUtilization = apiUtilization;
    snapshot_.resource.databaseUtilization = databaseUtilization;
}

void Metrics::setSimulationSpeed(double speed)
{
    snapshot_.simulationSpeed = speed;
}

void Metrics::setRuntimeSystemCounts(int enabledSystemCount, int initializedSystemCount)
{
    snapshot_.observability.enabledSystemCount = enabledSystemCount;
    snapshot_.observability.initializedSystemCount = initializedSystemCount;
}

void Metrics::setComplexity(double current, double recommendedThreshold)
{
    snapshot_.complexity.current = current;
    snapshot_.complexity.recommendedThreshold = recommendedThreshold;
    MetricsAggregator::update(snapshot_);
}

void Metrics::setPressureState(const PressureState& state)
{
    snapshot_.pressureState = state;
    snapshot_.pressureState.frontend.assetWeight = clamp01(snapshot_.pressureState.frontend.assetWeight);
    snapshot_.pressureState.frontend.renderComplexity = clamp01(snapshot_.pressureState.frontend.renderComplexity);
    snapshot_.pressureState.frontend.cacheEfficiency = clamp01(snapshot_.pressureState.frontend.cacheEfficiency);
    snapshot_.pressureState.frontend.realtimeIntensity = clamp01(snapshot_.pressureState.frontend.realtimeIntensity);
    snapshot_.pressureState.frontend.sessionPersistence = clamp01(snapshot_.pressureState.frontend.sessionPersistence);
    snapshot_.pressureState.frontend.mobileCompatibility = clamp01(snapshot_.pressureState.frontend.mobileCompatibility);
    snapshot_.pressureState.backend.requestLoad = clamp01(snapshot_.pressureState.backend.requestLoad);
    snapshot_.pressureState.backend.queuePressure = clamp01(snapshot_.pressureState.backend.queuePressure);
    snapshot_.pressureState.backend.computeIntensity = clamp01(snapshot_.pressureState.backend.computeIntensity);
    snapshot_.pressureState.backend.serviceFragmentation = clamp01(snapshot_.pressureState.backend.serviceFragmentation);
    snapshot_.pressureState.network.bandwidthPressure = clamp01(snapshot_.pressureState.network.bandwidthPressure);
    snapshot_.pressureState.network.latencySensitivity = clamp01(snapshot_.pressureState.network.latencySensitivity);
    snapshot_.pressureState.network.trafficBurstiness = clamp01(snapshot_.pressureState.network.trafficBurstiness);
    snapshot_.pressureState.database.readPressure = clamp01(snapshot_.pressureState.database.readPressure);
    snapshot_.pressureState.database.writePressure = clamp01(snapshot_.pressureState.database.writePressure);
    snapshot_.pressureState.database.contention = clamp01(snapshot_.pressureState.database.contention);
    snapshot_.pressureState.database.replicationLag = clamp01(snapshot_.pressureState.database.replicationLag);
    snapshot_.pressureState.runtime.cpuPressure = clamp01(snapshot_.pressureState.runtime.cpuPressure);
    snapshot_.pressureState.runtime.memoryPressure = clamp01(snapshot_.pressureState.runtime.memoryPressure);
    snapshot_.pressureState.runtime.allocationOrGcPressure = clamp01(snapshot_.pressureState.runtime.allocationOrGcPressure);
    snapshot_.pressureState.runtime.schedulingPressure = clamp01(snapshot_.pressureState.runtime.schedulingPressure);
    MetricsAggregator::update(snapshot_);
}

void Metrics::setActivePressureSignals(std::vector<PressureContextSignal> signals)
{
    snapshot_.activePressureSignals = std::move(signals);
}

void Metrics::update(double dt)
{
    windowElapsed_ += dt;
    if (windowElapsed_ < 1.0) {
        MetricsAggregator::update(snapshot_);
        return;
    }

    snapshot_.inputRatePerSecond = generatedInWindow_ / windowElapsed_;
    snapshot_.processedPerSecond = processedInWindow_ / windowElapsed_;
    snapshot_.timeoutRatePerSecond = timedOutInWindow_ / windowElapsed_;
    snapshot_.retryRatePerSecond = retriesInWindow_ / windowElapsed_;
    snapshot_.flow.inputRatePerSecond = snapshot_.inputRatePerSecond;
    snapshot_.flow.processedPerSecond = snapshot_.processedPerSecond;
    snapshot_.reliability.timeoutRatePerSecond = snapshot_.timeoutRatePerSecond;
    snapshot_.reliability.retryRatePerSecond = snapshot_.retryRatePerSecond;
    if (cacheLookupsInWindow_ > 0) {
        snapshot_.cacheHitRate = static_cast<double>(cacheHitsInWindow_) / cacheLookupsInWindow_;
    } else {
        snapshot_.cacheHitRate = 0.0;
    }
    if (latencySamplesInWindow_ > 0) {
        snapshot_.averageLatencySeconds = latencySumInWindow_ / latencySamplesInWindow_;
    }
    snapshot_.flow.averageLatencySeconds = snapshot_.averageLatencySeconds;

    windowElapsed_ = 0.0;
    generatedInWindow_ = 0;
    processedInWindow_ = 0;
    timedOutInWindow_ = 0;
    retriesInWindow_ = 0;
    cacheHitsInWindow_ = 0;
    cacheLookupsInWindow_ = 0;
    latencySumInWindow_ = 0.0;
    latencySamplesInWindow_ = 0;

    MetricsAggregator::update(snapshot_);
}

const MetricsSnapshot& Metrics::snapshot() const
{
    return snapshot_;
}
