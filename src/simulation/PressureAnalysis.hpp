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
    std::deque<PressureEvent> recentEvents;
};

class PressureAnalysisSystem {
public:
    void reset();
    void update(double timeSeconds, double dt, const InfrastructureGraph& graph, const MetricsSnapshot& metrics);

    [[nodiscard]] const PressureSnapshot& snapshot() const;
    [[nodiscard]] const NodePressure* pressureForNode(int nodeId) const;

private:
    void addEvent(double timeSeconds, PressureCategory category, int nodeId, std::string summary);
    [[nodiscard]] bool eventCooldownElapsed(PressureCategory category, double timeSeconds) const;

    PressureSnapshot snapshot_{};
    std::unordered_map<int, int> previousQueueDepths_;
    std::unordered_map<PressureCategory, double> lastEventTimes_;
};

const char* pressureCategoryName(PressureCategory category);
