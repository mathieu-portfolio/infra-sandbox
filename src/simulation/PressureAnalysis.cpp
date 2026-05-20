#include "simulation/PressureAnalysis.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
double clamp01(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

PressureCategory dominantFor(const Node& node, const MetricsSnapshot& metrics, const NodePressure& pressure, const PressureAnalysisConfig& config)
{
    if (pressure.retryContribution > config.retryDominantThreshold) {
        return PressureCategory::RetryPressure;
    }
    if (pressure.timeoutContribution > config.failureDominantThreshold) {
        return PressureCategory::FailurePressure;
    }
    if (node.type == NodeType::Database && pressure.queuePressure > config.persistenceQueueThreshold) {
        return PressureCategory::PersistencePressure;
    }
    if (pressure.latencyContribution > config.latencyContributionThreshold || metrics.averageLatencySeconds > config.averageLatencyThreshold) {
        return PressureCategory::LatencyPressure;
    }
    if (pressure.queuePressure > config.queuePressureThreshold || pressure.queueGrowthPerSecond > config.queueGrowthThreshold) {
        return PressureCategory::QueuePressure;
    }
    if (pressure.computePressure > config.computePressureThreshold) {
        return PressureCategory::ComputePressure;
    }
    if (metrics.inputRatePerSecond > metrics.processedPerSecond + config.trafficBacklogThreshold) {
        return PressureCategory::TrafficPressure;
    }
    return PressureCategory::None;
}

std::string explanationFor(const Node& node, const MetricsSnapshot& metrics, const NodePressure& pressure, const PressureAnalysisConfig& config)
{
    switch (pressure.dominant) {
    case PressureCategory::PersistencePressure:
        return config.persistenceNodeExplanation;
    case PressureCategory::RetryPressure:
        return config.retryNodeExplanation;
    case PressureCategory::LatencyPressure:
        return node.hasGeoLocation && metrics.averageLatencySeconds > 1.5
            ? config.geoLatencyExplanation
            : config.localLatencyExplanation;
    case PressureCategory::QueuePressure:
        return config.queueNodeExplanation;
    case PressureCategory::ComputePressure:
        return config.computeNodeExplanation;
    case PressureCategory::FailurePressure:
        return config.failureNodeExplanation;
    case PressureCategory::TrafficPressure:
        return config.trafficNodeExplanation;
    case PressureCategory::None:
        break;
    }
    if (node.isProcessor()) {
        return config.processorNoPressureExplanation;
    }
    return config.trafficSourceExplanation;
}
}

void PressureAnalysisSystem::reset()
{
    snapshot_ = {};
    previousQueueDepths_.clear();
    lastEventTimes_.clear();
}

void PressureAnalysisSystem::setConfig(PressureAnalysisConfig config)
{
    config_ = std::move(config);
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
        pressure.queuePressure = std::max(node.queuePressure, clamp01(static_cast<double>(node.queue.size()) / std::max(1.0, node.processingCapacityPerSecond * config_.queueCapacityWindow)));
        pressure.computePressure = std::max(node.currentUtilization, node.stressScore);
        pressure.latencyContribution = std::max(node.latencyPressure, clamp01(node.averageQueueWaitSeconds + node.currentUtilization * 0.35));
        pressure.timeoutContribution = std::max(node.timeoutPressure, clamp01(metrics.timeoutRatePerSecond / config_.timeoutRateScale) * pressure.queuePressure);
        pressure.retryContribution = std::max(node.retryPressure, clamp01(metrics.retryRatePerSecond / config_.retryRateScale) * std::max(pressure.queuePressure, node.type == NodeType::Database ? config_.databaseRetryFloor : config_.processorRetryFloor));
        double downstreamPressure = 0.0;
        int downstreamCount = 0;
        const Node* strongestDependency = nullptr;
        double strongestDependencyPressure = -1.0;
        for (const auto& link : graph.links()) {
            if (!link.enabled || link.sourceNodeId != node.id) {
                continue;
            }
            if (const Node* target = graph.node(link.targetNodeId)) {
                const double targetPressure = std::max({target->queuePressure, target->stressScore, target->propagatedPressure, target->propagatedInstability});
                downstreamPressure += targetPressure;
                ++downstreamCount;
                if (targetPressure > strongestDependencyPressure) {
                    strongestDependencyPressure = targetPressure;
                    strongestDependency = target;
                }
            }
        }
        pressure.dependencyPressure = std::max(node.propagatedPressure, downstreamCount > 0 ? downstreamPressure / downstreamCount : 0.0);
        pressure.instability = std::max({pressure.queuePressure, pressure.computePressure, pressure.timeoutContribution, pressure.retryContribution, pressure.dependencyPressure, node.propagatedInstability, node.stressScore});
        pressure.dominant = dominantFor(node, metrics, pressure, config_);
        pressure.explanation = explanationFor(node, metrics, pressure, config_);

        if (pressure.dependencyPressure > config_.dependencyPressureThreshold) {
            pressure.suspectedSource = strongestDependency != nullptr ? strongestDependency->name : "Dependency instability";
            pressure.pressureChain = strongestDependency != nullptr
                ? strongestDependency->name + " -> " + node.name
                : std::string("Dependency -> ") + node.name;
            pressure.diagnosisConfidence = std::clamp(0.40 + pressure.dependencyPressure * 0.42 + node.propagatedInstability * 0.18, 0.0, 0.86);
        } else if (pressure.retryContribution > config_.retryDominantThreshold) {
            pressure.suspectedSource = "Retry amplification";
            pressure.pressureChain = "Latency -> Retries -> Traffic";
            pressure.diagnosisConfidence = std::clamp(0.48 + pressure.retryContribution * 0.38, 0.0, 0.82);
        } else if (pressure.queuePressure > config_.queuePressureThreshold) {
            pressure.suspectedSource = node.type == NodeType::QueueBroker ? "Queue backlog" : node.name;
            pressure.pressureChain = node.type == NodeType::QueueBroker ? "Worker capacity -> Queue backlog" : "Queue -> Latency";
            pressure.diagnosisConfidence = std::clamp(0.42 + pressure.queuePressure * 0.35, 0.0, 0.78);
        }

        if (pressure.dependencyPressure > config_.dependencyPressureThreshold) {
            pressure.dependencySummary = config_.dependencyPressureSummary;
        } else if (downstreamCount > 0) {
            pressure.dependencySummary = config_.dependencyStableSummary;
        } else {
            pressure.dependencySummary = config_.noDependencySummary;
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

    if (database != nullptr && database->queue.size() > config_.databaseQueueHintThreshold && database->currentUtilization > config_.databaseUtilizationHintThreshold) {
        snapshot_.hints.push_back(config_.databaseQueueHint);
        snapshot_.hints.push_back(config_.databaseLatencyHint);
        snapshot_.explanations.push_back(config_.databaseExplanation);
        snapshot_.suspiciousPatterns.push_back(config_.databasePattern);
        if (eventCooldownElapsed(PressureCategory::PersistencePressure, timeSeconds)) {
            addEvent(timeSeconds, PressureCategory::PersistencePressure, database->id, "Database pressure detected");
        }
    }

    if (api != nullptr && api->queue.size() > config_.apiQueueHintThreshold && (database == nullptr || database->queue.size() <= api->queue.size())) {
        snapshot_.hints.push_back(config_.apiQueueHint);
        snapshot_.explanations.push_back(config_.apiExplanation);
        snapshot_.suspiciousPatterns.push_back(config_.apiPattern);
        if (eventCooldownElapsed(PressureCategory::QueuePressure, timeSeconds)) {
            addEvent(timeSeconds, PressureCategory::QueuePressure, api->id, "API queue spike");
        }
    }

    if (metrics.retryRatePerSecond > config_.retryRateHintThreshold && metrics.timeoutRatePerSecond > config_.timeoutRateHintThreshold) {
        snapshot_.hints.push_back(config_.retryHint);
        snapshot_.suspiciousPatterns.push_back(config_.retryPattern);
        if (eventCooldownElapsed(PressureCategory::RetryPressure, timeSeconds)) {
            addEvent(timeSeconds, PressureCategory::RetryPressure, snapshot_.retryAmplificationNodeId, "Retry amplification");
        }
    }

    if (metrics.averageLatencySeconds > config_.averageLatencyThreshold) {
        snapshot_.hints.push_back(config_.latencyHint);
        snapshot_.explanations.push_back(config_.latencyExplanation);
        if (eventCooldownElapsed(PressureCategory::LatencyPressure, timeSeconds)) {
            addEvent(timeSeconds, PressureCategory::LatencyPressure, snapshot_.dominantLatencyNodeId, "Latency wave");
        }
    }

    if (metrics.timeoutRatePerSecond > config_.failureTimeoutRateThreshold && eventCooldownElapsed(PressureCategory::FailurePressure, timeSeconds)) {
        addEvent(timeSeconds, PressureCategory::FailurePressure, snapshot_.mostUnstableNodeId, "Timeout wave");
    }

    if (metrics.cacheHitRate > config_.cacheHitSurgeThreshold && eventCooldownElapsed(PressureCategory::PersistencePressure, timeSeconds)) {
        addEvent(timeSeconds, PressureCategory::PersistencePressure, -1, "Cache hit surge");
        snapshot_.hints.push_back(config_.cacheHint);
    }

    if (snapshot_.dominantPressure != PressureCategory::None) {
        snapshot_.pressureHistory.push_back({timeSeconds, snapshot_.dominantPressure, snapshot_.mostUnstableNodeId, pressureCategoryName(snapshot_.dominantPressure)});
    }
    while (snapshot_.pressureHistory.size() > config_.pressureHistoryLimit) {
        snapshot_.pressureHistory.pop_front();
    }

    if (snapshot_.pressureHistory.size() >= static_cast<std::size_t>(config_.recurringPressureSampleCount)) {
        const auto latest = snapshot_.pressureHistory.back().category;
        const int repeated = static_cast<int>(std::count_if(snapshot_.pressureHistory.begin(), snapshot_.pressureHistory.end(), [latest](const PressureEvent& event) {
            return event.category == latest;
        }));
        if (repeated >= config_.recurringPressureMinimum) {
            snapshot_.suspiciousPatterns.push_back(std::string(pressureCategoryName(latest)) + " pressure is recurring over the short-term window.");
        }
    }

    while (snapshot_.recentEvents.size() > config_.recentEventLimit) {
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
    return it == lastEventTimes_.end() || timeSeconds - it->second > config_.eventCooldownSeconds;
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
