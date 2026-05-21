#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class GlobalMetricId {
    UserExperience,
    InfrastructurePressure,
    Reliability,
    Complexity,
    Scalability,
};

enum class MetricContributionDomain {
    Frontend,
    Persistence,
    Runtime,
};

struct MetricContribution {
    GlobalMetricId target = GlobalMetricId::UserExperience;
    MetricContributionDomain domain = MetricContributionDomain::Frontend;
    const char* label = "";
    double amount = 0.0;
};

struct PressureContextSignal {
    MetricContributionDomain domain = MetricContributionDomain::Frontend;
    std::string name;
    std::string summary;
    bool temporary = false;
};

struct GlobalMetrics {
    double userExperience = 100.0;
    double infrastructurePressure = 0.0;
    double reliability = 100.0;
    double complexity = 0.0;
    double scalability = 100.0;
};

struct FrontendMetrics {
    double renderLatency = 0.0;
    double framePressure = 0.0;
    double assetBandwidth = 0.0;
    double interactionDelay = 0.0;
    double perceivedLatency = 0.0;
    double websocketPressure = 0.0;
    double sessionWarmth = 0.0;
    double sessionStalenessRisk = 0.0;
};

struct BackendMetrics {
    double requestLoad = 0.0;
    double queuePressure = 0.0;
    double computeIntensity = 0.0;
    double serviceFragmentation = 0.0;
    double reliabilityRisk = 0.0;
};

struct NetworkMetrics {
    double bandwidthPressure = 0.0;
    double latencySensitivity = 0.0;
    double trafficBurstiness = 0.0;
    double deliveryPressure = 0.0;
};

struct FrontendState {
    double assetWeight = 0.0;
    double renderComplexity = 0.0;
    double cacheEfficiency = 0.0;
    double realtimeIntensity = 0.0;
    double sessionPersistence = 0.0;
    double mobileCompatibility = 0.0;
};

struct BackendState {
    double requestLoad = 0.0;
    double queuePressure = 0.0;
    double computeIntensity = 0.0;
    double serviceFragmentation = 0.0;
};

struct NetworkState {
    double bandwidthPressure = 0.0;
    double latencySensitivity = 0.0;
    double trafficBurstiness = 0.0;
};

struct PressureState {
    FrontendState frontend;
    BackendState backend;
    NetworkState network;
};

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
    double current = 0.0;
    double recommendedThreshold = 10.0;
};

struct ObservabilityMetrics {
    int enabledSystemCount = 0;
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
    GlobalMetrics global;
    FrontendMetrics frontend;
    BackendMetrics backend;
    NetworkMetrics network;
    PressureState pressureState;
    std::vector<PressureContextSignal> activePressureSignals;
    std::vector<MetricContribution> contributions;
};

class MetricsAggregator {
public:
    static void update(MetricsSnapshot& snapshot);
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
    void setRuntimeSystemCounts(int enabledSystemCount, int initializedSystemCount);
    void setComplexity(double current, double recommendedThreshold);
    void setPressureState(const PressureState& state);
    void setActivePressureSignals(std::vector<PressureContextSignal> signals);
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
