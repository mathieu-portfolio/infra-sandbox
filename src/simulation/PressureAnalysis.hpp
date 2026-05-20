#pragma once

#include "simulation/InfrastructureGraph.hpp"
#include "simulation/Metrics.hpp"

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

enum class PressureCategory {
    None,
    TrafficPressure,
    QueuePressure,
    ComputePressure,
    PersistencePressure,
    RetryPressure,
    LatencyPressure,
    FailurePressure
};

struct NodePressure {
    int nodeId = -1;
    PressureCategory dominant = PressureCategory::None;
    double queueGrowthPerSecond = 0.0;
    double queuePressure = 0.0;
    double computePressure = 0.0;
    double latencyContribution = 0.0;
    double timeoutContribution = 0.0;
    double retryContribution = 0.0;
    double instability = 0.0;
    double dependencyPressure = 0.0;
    std::string explanation;
    std::string dependencySummary;
    std::string suspectedSource;
    std::string pressureChain;
    double diagnosisConfidence = 0.0;
};

struct PressureEvent {
    double timeSeconds = 0.0;
    PressureCategory category = PressureCategory::None;
    int nodeId = -1;
    std::string summary;
};

struct PressureSnapshot {
    PressureCategory dominantPressure = PressureCategory::None;
    int topOverloadedNodeId = -1;
    int highestQueueGrowthNodeId = -1;
    int dominantLatencyNodeId = -1;
    int mostUnstableNodeId = -1;
    int retryAmplificationNodeId = -1;
    std::vector<NodePressure> nodes;
    std::vector<std::string> hints;
    std::vector<std::string> explanations;
    std::vector<std::string> suspiciousPatterns;
    std::deque<PressureEvent> pressureHistory;
    std::deque<PressureEvent> recentEvents;
};

struct PressureAnalysisConfig {
    double retryDominantThreshold = 0.45;
    double failureDominantThreshold = 0.45;
    double persistenceQueueThreshold = 0.35;
    double latencyContributionThreshold = 2.0;
    double averageLatencyThreshold = 3.0;
    double queuePressureThreshold = 0.35;
    double queueGrowthThreshold = 0.5;
    double computePressureThreshold = 0.75;
    double trafficBacklogThreshold = 2.0;
    double queueCapacityWindow = 2.0;
    double timeoutRateScale = 4.0;
    double retryRateScale = 3.0;
    double databaseRetryFloor = 0.45;
    double processorRetryFloor = 0.25;
    double dependencyPressureThreshold = 0.45;
    double databaseQueueHintThreshold = 3.0;
    double databaseUtilizationHintThreshold = 0.75;
    double apiQueueHintThreshold = 4.0;
    double retryRateHintThreshold = 0.5;
    double timeoutRateHintThreshold = 0.2;
    double failureTimeoutRateThreshold = 1.0;
    double cacheHitSurgeThreshold = 0.45;
    double eventCooldownSeconds = 6.0;
    std::size_t pressureHistoryLimit = 24;
    std::size_t recentEventLimit = 8;
    int recurringPressureSampleCount = 4;
    int recurringPressureMinimum = 3;
    std::string persistenceNodeExplanation = "Requests are mostly waiting on downstream persistence operations.";
    std::string retryNodeExplanation = "Traffic bursts appear to be amplifying retries near this path.";
    std::string geoLatencyExplanation = "Latency is likely influenced by queueing and cross-region communication.";
    std::string localLatencyExplanation = "Latency is rising as local work waits longer in the system.";
    std::string queueNodeExplanation = "Queue growth is outpacing the service's current processing rate.";
    std::string computeNodeExplanation = "The node is spending most of its available processing capacity.";
    std::string failureNodeExplanation = "Timeouts indicate requests are exceeding the current recovery window.";
    std::string trafficNodeExplanation = "Incoming demand is higher than completed throughput.";
    std::string processorNoPressureExplanation = "No dominant local pressure detected; inspect dependencies for shifted pressure.";
    std::string trafficSourceExplanation = "Traffic source is contributing demand into the dependency path.";
    std::string dependencyPressureSummary = "Downstream dependencies show visible queue pressure.";
    std::string dependencyStableSummary = "Downstream dependencies are not currently dominant.";
    std::string noDependencySummary = "No downstream dependencies from this node.";
    std::string databaseQueueHint = "Queue pressure is accumulating near Database.";
    std::string databaseLatencyHint = "Downstream persistence pressure can feed API latency.";
    std::string databaseExplanation = "Repeated reads are heavily stressing persistence.";
    std::string databasePattern = "Persistence queue and utilization are high together.";
    std::string apiQueueHint = "API queue growth is outpacing processing capacity.";
    std::string apiExplanation = "API work is arriving faster than local compute can drain it.";
    std::string apiPattern = "API queue growth may be local compute pressure or downstream wait.";
    std::string retryHint = "Retry traffic appears to amplify overload.";
    std::string retryPattern = "Retries and timeouts are rising together.";
    std::string latencyHint = "Latency pressure is dominated by queued work.";
    std::string latencyExplanation = "Most latency currently comes from queueing or long dependency paths.";
    std::string cacheHint = "Repeated expensive requests are being absorbed by cache.";
};

class PressureAnalysisSystem {
public:
    void reset();
    void setConfig(PressureAnalysisConfig config);
    void update(double timeSeconds, double dt, const InfrastructureGraph& graph, const MetricsSnapshot& metrics);

    [[nodiscard]] const PressureSnapshot& snapshot() const;
    [[nodiscard]] const NodePressure* pressureForNode(int nodeId) const;

private:
    void addEvent(double timeSeconds, PressureCategory category, int nodeId, std::string summary);
    [[nodiscard]] bool eventCooldownElapsed(PressureCategory category, double timeSeconds) const;

    PressureSnapshot snapshot_{};
    PressureAnalysisConfig config_{};
    std::unordered_map<int, int> previousQueueDepths_;
    std::unordered_map<PressureCategory, double> lastEventTimes_;
};

const char* pressureCategoryName(PressureCategory category);
