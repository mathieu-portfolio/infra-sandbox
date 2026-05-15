#pragma once

#include <cstdint>

struct MetricsSnapshot {
    double inputRatePerSecond = 0.0;
    double processedPerSecond = 0.0;
    double timeoutRatePerSecond = 0.0;
    double averageLatencySeconds = 0.0;
    double backendUtilization = 0.0;
    int backendQueueDepth = 0;
    std::uint64_t totalGenerated = 0;
    std::uint64_t totalProcessed = 0;
    std::uint64_t totalTimedOut = 0;
};

class Metrics {
public:
    void reset();
    void recordGenerated();
    void recordProcessed(double latencySeconds);
    void recordTimedOut(double latencySeconds);
    void setBackendState(int queueDepth, double utilization);
    void update(double dt);

    [[nodiscard]] const MetricsSnapshot& snapshot() const;

private:
    MetricsSnapshot snapshot_{};
    double windowElapsed_ = 0.0;
    int generatedInWindow_ = 0;
    int processedInWindow_ = 0;
    int timedOutInWindow_ = 0;
    double latencySumInWindow_ = 0.0;
    int latencySamplesInWindow_ = 0;
};
