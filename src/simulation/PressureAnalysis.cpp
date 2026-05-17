#include "simulation/PressureAnalysis.hpp"

#include <algorithm>
#include <cmath>

namespace {
double clamp01(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

PressureCategory dominantFor(const Node& node, const MetricsSnapshot& metrics, const NodePressure& pressure)
{
    if (pressure.retryContribution > 0.45) {
        return PressureCategory::RetryPressure;
    }
    if (pressure.timeoutContribution > 0.45) {
        return PressureCategory::FailurePressure;
    }
    if (node.type == NodeType::Database && pressure.queuePressure > 0.35) {
        return PressureCategory::PersistencePressure;
    }
    if (pressure.latencyContribution > 2.0 || metrics.averageLatencySeconds > 3.0) {
        return PressureCategory::LatencyPressure;
    }
    if (pressure.queuePressure > 0.35 || pressure.queueGrowthPerSecond > 0.5) {
        return PressureCategory::QueuePressure;
    }
    if (pressure.computePressure > 0.75) {
        return PressureCategory::ComputePressure;
    }
    if (metrics.inputRatePerSecond > metrics.processedPerSecond + 2.0) {
        return PressureCategory::TrafficPressure;
    }
    return PressureCategory::None;
}

std::string explanationFor(const Node& node, const MetricsSnapshot& metrics, const NodePressure& pressure)
{
    switch (pressure.dominant) {
    case PressureCategory::PersistencePressure:
        return "Requests are mostly waiting on downstream persistence operations.";
    case PressureCategory::RetryPressure:
        return "Traffic bursts appear to be amplifying retries near this path.";
    case PressureCategory::LatencyPressure:
        return node.hasGeoLocation && metrics.averageLatencySeconds > 1.5
            ? "Latency is likely influenced by queueing and cross-region communication."
            : "Latency is rising as local work waits longer in the system.";
    case PressureCategory::QueuePressure:
        return "Queue growth is outpacing the service's current processing rate.";
    case PressureCategory::ComputePressure:
        return "The node is spending most of its available processing capacity.";
    case PressureCategory::FailurePressure:
        return "Timeouts indicate requests are exceeding the current recovery window.";
    case PressureCategory::TrafficPressure:
        return "Incoming demand is higher than completed throughput.";
    case PressureCategory::None:
        break;
    }
    if (node.isProcessor()) {
        return "No dominant local pressure detected; inspect dependencies for shifted pressure.";
    }
    return "Traffic source is contributing demand into the dependency path.";
}
}

void PressureAnalysisSystem::reset()
{
    snapshot_ = {};
    previousQueueDepths_.clear();
    lastEventTimes_.clear();
}

void PressureAnalysisSystem::update(double timeSeconds, double dt, const InfrastructureGraph& graph, const MetricsSnapshot& metrics)
{
    snapshot_.nodes.clear();
    snapshot_.hints.clear();
    snapshot_.explanations.clear();
    snapshot_.suspiciousPatterns.clear();

    double topOverload = -1.0;
    double topGrowth = -1000000.0;
    double topLatency = -1.0;
    double topInstability = -1.0;
    double topRetry = -1.0;

    for (const auto& node : graph.nodes()) {
        const auto previousDepthIt = previousQueueDepths_.find(node.id);
        const int previousDepth = previousDepthIt == previousQueueDepths_.end()
            ? static_cast<int>(node.queue.size())
            : previousDepthIt->second;

        NodePressure pressure;
        pressure.nodeId = node.id;
        pressure.queueGrowthPerSecond = dt > 0.0
            ? (static_cast<double>(node.queue.size()) - previousDepth) / dt
            : 0.0;
        pressure.queuePressure = clamp01(static_cast<double>(node.queue.size()) / std::max(1.0, node.processingCapacityPerSecond * 2.0));
        pressure.computePressure = node.currentUtilization;
        pressure.latencyContribution = node.averageQueueWaitSeconds + node.currentUtilization * 0.35;
        pressure.timeoutContribution = clamp01(metrics.timeoutRatePerSecond / 4.0) * pressure.queuePressure;
        pressure.retryContribution = clamp01(metrics.retryRatePerSecond / 3.0) * std::max(pressure.queuePressure, node.type == NodeType::Database ? 0.45 : 0.25);
        double downstreamPressure = 0.0;
        int downstreamCount = 0;
        for (const auto& link : graph.links()) {
            if (!link.enabled || link.sourceNodeId != node.id) {
                continue;
            }
            if (const Node* target = graph.node(link.targetNodeId)) {
                downstreamPressure += clamp01(static_cast<double>(target->queue.size()) / std::max(1.0, target->processingCapacityPerSecond * 2.0));
                ++downstreamCount;
            }
        }
        pressure.dependencyPressure = downstreamCount > 0 ? downstreamPressure / downstreamCount : 0.0;
        pressure.instability = std::max({pressure.queuePressure, pressure.computePressure, pressure.timeoutContribution, pressure.retryContribution});
        pressure.dominant = dominantFor(node, metrics, pressure);
        pressure.explanation = explanationFor(node, metrics, pressure);
        if (pressure.dependencyPressure > 0.45) {
            pressure.dependencySummary = "Downstream dependencies show visible queue pressure.";
        } else if (downstreamCount > 0) {
            pressure.dependencySummary = "Downstream dependencies are not currently dominant.";
        } else {
            pressure.dependencySummary = "No downstream dependencies from this node.";
        }
        snapshot_.nodes.push_back(pressure);

        if (pressure.computePressure > topOverload) {
            topOverload = pressure.computePressure;
            snapshot_.topOverloadedNodeId = node.id;
        }
        if (pressure.queueGrowthPerSecond > topGrowth) {
            topGrowth = pressure.queueGrowthPerSecond;
            snapshot_.highestQueueGrowthNodeId = node.id;
        }
        if (pressure.latencyContribution > topLatency) {
            topLatency = pressure.latencyContribution;
            snapshot_.dominantLatencyNodeId = node.id;
        }
        if (pressure.instability > topInstability) {
            topInstability = pressure.instability;
            snapshot_.mostUnstableNodeId = node.id;
        }
        if (pressure.retryContribution > topRetry) {
            topRetry = pressure.retryContribution;
            snapshot_.retryAmplificationNodeId = node.id;
        }

        previousQueueDepths_[node.id] = static_cast<int>(node.queue.size());
    }

    snapshot_.dominantPressure = PressureCategory::None;
    if (const auto* unstable = pressureForNode(snapshot_.mostUnstableNodeId)) {
        snapshot_.dominantPressure = unstable->dominant;
    }

    const Node* database = nullptr;
    const Node* api = nullptr;
    for (const auto& node : graph.nodes()) {
        if (node.type == NodeType::Database) {
            database = &node;
        } else if (node.type == NodeType::ApiService) {
            api = &node;
        }
    }

    if (database != nullptr && database->queue.size() > 3 && database->currentUtilization > 0.75) {
        snapshot_.hints.push_back("Queue pressure is accumulating near Database.");
        snapshot_.hints.push_back("Downstream persistence pressure can feed API latency.");
        snapshot_.explanations.push_back("Repeated reads are heavily stressing persistence.");
        snapshot_.suspiciousPatterns.push_back("Persistence queue and utilization are high together.");
        if (eventCooldownElapsed(PressureCategory::PersistencePressure, timeSeconds)) {
            addEvent(timeSeconds, PressureCategory::PersistencePressure, database->id, "Database pressure detected");
        }
    }

    if (api != nullptr && api->queue.size() > 4 && (database == nullptr || database->queue.size() <= api->queue.size())) {
        snapshot_.hints.push_back("API queue growth is outpacing processing capacity.");
        snapshot_.explanations.push_back("API work is arriving faster than local compute can drain it.");
        snapshot_.suspiciousPatterns.push_back("API queue growth may be local compute pressure or downstream wait.");
        if (eventCooldownElapsed(PressureCategory::QueuePressure, timeSeconds)) {
            addEvent(timeSeconds, PressureCategory::QueuePressure, api->id, "API queue spike");
        }
    }

    if (metrics.retryRatePerSecond > 0.5 && metrics.timeoutRatePerSecond > 0.2) {
        snapshot_.hints.push_back("Retry traffic appears to amplify overload.");
        snapshot_.suspiciousPatterns.push_back("Retries and timeouts are rising together.");
        if (eventCooldownElapsed(PressureCategory::RetryPressure, timeSeconds)) {
            addEvent(timeSeconds, PressureCategory::RetryPressure, snapshot_.retryAmplificationNodeId, "Retry amplification");
        }
    }

    if (metrics.averageLatencySeconds > 3.0) {
        snapshot_.hints.push_back("Latency pressure is dominated by queued work.");
        snapshot_.explanations.push_back("Most latency currently comes from queueing or long dependency paths.");
        if (eventCooldownElapsed(PressureCategory::LatencyPressure, timeSeconds)) {
            addEvent(timeSeconds, PressureCategory::LatencyPressure, snapshot_.dominantLatencyNodeId, "Latency wave");
        }
    }

    if (metrics.timeoutRatePerSecond > 1.0 && eventCooldownElapsed(PressureCategory::FailurePressure, timeSeconds)) {
        addEvent(timeSeconds, PressureCategory::FailurePressure, snapshot_.mostUnstableNodeId, "Timeout wave");
    }

    if (metrics.cacheHitRate > 0.45 && eventCooldownElapsed(PressureCategory::PersistencePressure, timeSeconds)) {
        addEvent(timeSeconds, PressureCategory::PersistencePressure, -1, "Cache hit surge");
        snapshot_.hints.push_back("Repeated expensive requests are being absorbed by cache.");
    }

    if (snapshot_.dominantPressure != PressureCategory::None) {
        snapshot_.pressureHistory.push_back({timeSeconds, snapshot_.dominantPressure, snapshot_.mostUnstableNodeId, pressureCategoryName(snapshot_.dominantPressure)});
    }
    while (snapshot_.pressureHistory.size() > 24) {
        snapshot_.pressureHistory.pop_front();
    }

    if (snapshot_.pressureHistory.size() >= 4) {
        const auto latest = snapshot_.pressureHistory.back().category;
        const int repeated = static_cast<int>(std::count_if(snapshot_.pressureHistory.begin(), snapshot_.pressureHistory.end(), [latest](const PressureEvent& event) {
            return event.category == latest;
        }));
        if (repeated >= 3) {
            snapshot_.suspiciousPatterns.push_back(std::string(pressureCategoryName(latest)) + " pressure is recurring over the short-term window.");
        }
    }

    while (snapshot_.recentEvents.size() > 8) {
        snapshot_.recentEvents.pop_front();
    }
}

const PressureSnapshot& PressureAnalysisSystem::snapshot() const
{
    return snapshot_;
}

const NodePressure* PressureAnalysisSystem::pressureForNode(int nodeId) const
{
    for (const auto& pressure : snapshot_.nodes) {
        if (pressure.nodeId == nodeId) {
            return &pressure;
        }
    }
    return nullptr;
}

void PressureAnalysisSystem::addEvent(double timeSeconds, PressureCategory category, int nodeId, std::string summary)
{
    snapshot_.recentEvents.push_back({
        .timeSeconds = timeSeconds,
        .category = category,
        .nodeId = nodeId,
        .summary = std::move(summary),
    });
    lastEventTimes_[category] = timeSeconds;
}

bool PressureAnalysisSystem::eventCooldownElapsed(PressureCategory category, double timeSeconds) const
{
    const auto it = lastEventTimes_.find(category);
    return it == lastEventTimes_.end() || timeSeconds - it->second > 6.0;
}

const char* pressureCategoryName(PressureCategory category)
{
    switch (category) {
    case PressureCategory::None:
        return "None";
    case PressureCategory::TrafficPressure:
        return "Traffic";
    case PressureCategory::QueuePressure:
        return "Queue";
    case PressureCategory::ComputePressure:
        return "Compute";
    case PressureCategory::PersistencePressure:
        return "Persistence";
    case PressureCategory::RetryPressure:
        return "Retry";
    case PressureCategory::LatencyPressure:
        return "Latency";
    case PressureCategory::FailurePressure:
        return "Failure";
    }
    return "Unknown";
}
