#include "simulation/Metrics.hpp"

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

void Metrics::update(double dt)
{
    windowElapsed_ += dt;
    if (windowElapsed_ < 1.0) {
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
}

const MetricsSnapshot& Metrics::snapshot() const
{
    return snapshot_;
}
