#pragma once

#include <cstdint>

struct FlowMetrics {
    double inputRatePerSecond = 0.0;
    double processedPerSecond = 0.0;
    double averageLatencySeconds = 0.0;
};

struct ResourceMetrics {
    double apiUtilization = 0.0;
    double databaseUtilization = 0.0;
};

struct ReliabilityMetrics {
    double timeoutRatePerSecond = 0.0;
    double retryRatePerSecond = 0.0;
};

struct ComplexityMetrics {
    double placeholderScore = 0.0;
};

struct ObservabilityMetrics {
    int enabledLayerCount = 0;
    int initializedSystemCount = 0;
};

struct MetricsSnapshot {
    double inputRatePerSecond = 0.0;
    double processedPerSecond = 0.0;
    double timeoutRatePerSecond = 0.0;
    double retryRatePerSecond = 0.0;
    double averageLatencySeconds = 0.0;
    double apiUtilization = 0.0;
    double databaseUtilization = 0.0;
    double cacheHitRate = 0.0;
    double simulationSpeed = 1.0;
    int apiQueueDepth = 0;
    int databaseQueueDepth = 0;
    std::uint64_t totalGenerated = 0;
    std::uint64_t totalProcessed = 0;
    std::uint64_t totalTimedOut = 0;
    std::uint64_t totalRetries = 0;
    std::uint64_t totalCacheHits = 0;
    std::uint64_t totalCacheLookups = 0;
    FlowMetrics flow;
    ResourceMetrics resource;
    ReliabilityMetrics reliability;
    ComplexityMetrics complexity;
    ObservabilityMetrics observability;
};

class Metrics {
public:
    void reset();
    void recordGenerated();
    void recordProcessed(double latencySeconds);
    void recordTimedOut(double latencySeconds);
    void recordRetry();
    void recordCacheLookup(bool hit);
    void setNodeStates(int apiQueueDepth, double apiUtilization, int databaseQueueDepth, double databaseUtilization);
    void setSimulationSpeed(double speed);
    void setLayerSystemCounts(int enabledLayerCount, int initializedSystemCount);
    void update(double dt);

    [[nodiscard]] const MetricsSnapshot& snapshot() const;

private:
    MetricsSnapshot snapshot_{};
    double windowElapsed_ = 0.0;
    int generatedInWindow_ = 0;
    int processedInWindow_ = 0;
    int timedOutInWindow_ = 0;
    int retriesInWindow_ = 0;
    int cacheHitsInWindow_ = 0;
    int cacheLookupsInWindow_ = 0;
    double latencySumInWindow_ = 0.0;
    int latencySamplesInWindow_ = 0;
};
