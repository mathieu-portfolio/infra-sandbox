#include "simulation/metrics/Metrics.hpp"

#include <algorithm>
#include <cmath>

namespace {
double clampPercent(double value)
{
    return std::clamp(value, 0.0, 100.0);
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

    FrontendMetrics frontend;
    frontend.renderLatency = snapshot.averageLatencySeconds;
    frontend.framePressure = clampPercent(busiestNodeUtilization * 70.0 + queueDepth * 1.5);
    frontend.assetBandwidth = clampPercent(snapshot.inputRatePerSecond * 4.0);
    frontend.interactionDelay = snapshot.averageLatencySeconds + queueDepth * 0.01;
    frontend.perceivedLatency = frontend.renderLatency * 0.65 + frontend.interactionDelay * 0.35;
    frontend.websocketPressure = clampPercent(snapshot.retryRatePerSecond * 18.0 + trafficBacklog * 6.0);
    frontend.sessionWarmth = clampPercent(snapshot.cacheHitRate * 100.0);
    frontend.sessionStalenessRisk = clampPercent(snapshot.timeoutRatePerSecond * 24.0 + snapshot.retryRatePerSecond * 8.0);
    snapshot.frontend = frontend;

    const double latencyPenalty = clampPercent(frontend.perceivedLatency * 26.0);
    const double framePenalty = frontend.framePressure * 0.18;
    const double warmthBonus = frontend.sessionWarmth * 0.08;
    const double assetPressure = frontend.assetBandwidth * 0.22;
    const double websocketPressure = frontend.websocketPressure * 0.35;
    const double reliabilityPenalty = clampPercent(frontend.sessionStalenessRisk * 0.7 + snapshot.timeoutRatePerSecond * 8.0);
    const double persistenceComplexity = snapshot.totalCacheLookups > 0 ? 2.0 : 0.0;
    const double runtimeComplexity = snapshot.observability.enabledSystemCount > 0
        ? static_cast<double>(snapshot.observability.enabledSystemCount) * 0.25
        : 0.0;
    const double complexityPressure = snapshot.complexity.recommendedThreshold > 0.0
        ? (snapshot.complexity.current / snapshot.complexity.recommendedThreshold) * 100.0
        : snapshot.complexity.current * 10.0;

    GlobalMetrics global;
    global.userExperience = clampPercent(100.0 - latencyPenalty - framePenalty + warmthBonus);
    global.infrastructurePressure = clampPercent(busiestNodeUtilization * 55.0 + queueDepth * 1.5 + assetPressure + websocketPressure - frontend.sessionWarmth * 0.08);
    global.reliability = clampPercent(100.0 - reliabilityPenalty);
    global.complexity = clampPercent(complexityPressure + persistenceComplexity + runtimeComplexity);
    global.scalability = clampPercent(100.0 - global.infrastructurePressure * 0.45 - global.complexity * 0.2 + frontend.sessionWarmth * 0.08);
    snapshot.global = global;

    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Frontend, "Perceived latency", -latencyPenalty);
    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Frontend, "Frame pressure", -framePenalty);
    addContribution(snapshot, GlobalMetricId::UserExperience, MetricContributionDomain::Frontend, "Session warmth", warmthBonus);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Frontend, "Asset bandwidth", assetPressure);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Frontend, "Websocket pressure", websocketPressure);
    addContribution(snapshot, GlobalMetricId::InfrastructurePressure, MetricContributionDomain::Frontend, "Session warmth", -frontend.sessionWarmth * 0.08);
    addContribution(snapshot, GlobalMetricId::Reliability, MetricContributionDomain::Frontend, "Session staleness risk", -reliabilityPenalty);
    addContribution(snapshot, GlobalMetricId::Complexity, MetricContributionDomain::Persistence, "Persistence/session systems", persistenceComplexity);
    addContribution(snapshot, GlobalMetricId::Complexity, MetricContributionDomain::Runtime, "Runtime systems", runtimeComplexity);
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
