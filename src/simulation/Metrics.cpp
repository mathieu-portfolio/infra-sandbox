#include "simulation/Metrics.hpp"

void Metrics::reset()
{
    snapshot_ = {};
    windowElapsed_ = 0.0;
    generatedInWindow_ = 0;
    processedInWindow_ = 0;
    timedOutInWindow_ = 0;
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

void Metrics::setBackendState(int queueDepth, double utilization)
{
    snapshot_.backendQueueDepth = queueDepth;
    snapshot_.backendUtilization = utilization;
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
    if (latencySamplesInWindow_ > 0) {
        snapshot_.averageLatencySeconds = latencySumInWindow_ / latencySamplesInWindow_;
    }

    windowElapsed_ = 0.0;
    generatedInWindow_ = 0;
    processedInWindow_ = 0;
    timedOutInWindow_ = 0;
    latencySumInWindow_ = 0.0;
    latencySamplesInWindow_ = 0;
}

const MetricsSnapshot& Metrics::snapshot() const
{
    return snapshot_;
}
