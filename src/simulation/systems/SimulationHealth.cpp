#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


void Simulation::updatePropagatedPressure(double dt)
{
    const double smoothing = 1.0 - std::exp(-std::max(0.0, dt) / 2.0);
    std::unordered_map<int, double> incomingPressure;
    std::unordered_map<int, double> incomingInstability;

    auto localNodePressure = [](const Node& node) {
        return std::clamp(std::max({
            node.queuePressure,
            node.backlogPressure,
            node.currentUtilization,
            node.computePressure,
            node.memoryPressure,
            node.storagePressure,
            node.networkPressure,
            node.latencyPressure,
            node.timeoutPressure,
            node.retryPressure,
            node.stressScore
        }), 0.0, 1.0);
    };

    for (const auto& node : graph_.nodes()) {
        incomingPressure[node.id] = node.propagatedPressure * 0.72;
        incomingInstability[node.id] = node.propagatedInstability * 0.72;
    }

    for (const auto& link : graph_.links()) {
        if (!link.enabled) {
            continue;
        }
        const Node* source = graph_.node(link.sourceNodeId);
        const Node* target = graph_.node(link.targetNodeId);
        if (source == nullptr || target == nullptr) {
            continue;
        }

        const double targetPressure = localNodePressure(*target);
        const double sourcePressure = localNodePressure(*source);
        const double linkLatencyFactor = std::clamp(link.baseLatencySeconds / 2.0, 0.0, 1.0);
        const double linkLoadFactor = std::clamp(static_cast<double>(link.inFlightRequests.size()) / std::max(1.0, link.bandwidthPerSecond * 2.0), 0.0, 1.0);

        // Downstream trouble makes upstream callers unstable: a database or worker
        // bottleneck should show up as API dependency pressure, not merely as a
        // local database metric.
        const double downstreamContribution = std::clamp(targetPressure * (0.42 + linkLatencyFactor * 0.18) + linkLoadFactor * 0.20, 0.0, 1.0);
        incomingPressure[source->id] = std::max(incomingPressure[source->id], downstreamContribution);
        incomingInstability[source->id] = std::max(incomingInstability[source->id], std::clamp(downstreamContribution * 0.78 + target->retryPressure * 0.22, 0.0, 1.0));

        // Upstream floods also hurt downstream dependencies. This lets traffic
        // spikes and retry storms overload queues, workers, databases, and links.
        const double upstreamContribution = std::clamp(sourcePressure * 0.24 + source->retryPressure * 0.28 + linkLoadFactor * 0.28, 0.0, 1.0);
        incomingPressure[target->id] = std::max(incomingPressure[target->id], upstreamContribution);
        incomingInstability[target->id] = std::max(incomingInstability[target->id], std::clamp(upstreamContribution * 0.70 + source->propagatedInstability * 0.18, 0.0, 1.0));
    }

    for (auto& node : graph_.nodes()) {
        const double nextPressure = std::clamp(incomingPressure[node.id], 0.0, 1.0);
        const double nextInstability = std::clamp(incomingInstability[node.id], 0.0, 1.0);
        node.propagatedPressure += (nextPressure - node.propagatedPressure) * std::clamp(smoothing, 0.0, 1.0);
        node.propagatedInstability += (nextInstability - node.propagatedInstability) * std::clamp(smoothing * 0.85, 0.0, 1.0);
    }
}

void Simulation::updateNodeHealth(double dt)
{
    constexpr double kWindowSeconds = 4.0;
    const double smoothing = 1.0 - std::exp(-std::max(0.0, dt) / 2.5);
    const double decay = std::exp(-std::max(0.0, dt) / kWindowSeconds);

    auto smooth = [smoothing](double current, double measured) {
        return current + (measured - current) * std::clamp(smoothing, 0.0, 1.0);
    };

    for (auto& node : graph_.nodes()) {
        const double recentAttempts = std::max(1.0, node.recentCompleted + node.recentTimedOut);
        const double measuredTimeoutPressure = std::clamp(node.recentTimedOut / recentAttempts, 0.0, 1.0);
        const double measuredRetryPressure = std::clamp(node.recentRetries / std::max(1.0, node.recentGenerated), 0.0, 1.0);

        if (node.isProcessor()) {
            const double safeCapacity = std::max(1.0, node.processingCapacityPerSecond);
            const double queueFailureDepth = std::max(1.0, safeCapacity * std::max(1.0, scenario_.requestTimeoutSeconds) * 0.75);
            const double measuredQueuePressure = std::clamp(static_cast<double>(node.queue.size()) / queueFailureDepth, 0.0, 1.0);
            const double measuredLatencyPressure = std::clamp(node.averageQueueWaitSeconds / std::max(0.25, scenario_.requestTimeoutSeconds), 0.0, 1.0);
            const double measuredStress = std::clamp(
                node.currentUtilization * 0.32
                    + measuredQueuePressure * 0.28
                    + measuredLatencyPressure * 0.18
                    + measuredTimeoutPressure * 0.12
                    + measuredRetryPressure * 0.07
                    + node.propagatedPressure * 0.10
                    + node.propagatedInstability * 0.08,
                0.0,
                1.0);

            node.queuePressure = smooth(node.queuePressure, measuredQueuePressure);
            node.latencyPressure = smooth(node.latencyPressure, measuredLatencyPressure);
            node.timeoutPressure = smooth(node.timeoutPressure, measuredTimeoutPressure);
            node.retryPressure = smooth(node.retryPressure, measuredRetryPressure);
            node.stressScore = smooth(node.stressScore, measuredStress);
            node.healthScore = smooth(node.healthScore, 1.0 - measuredStress);
            node.experienceScore = node.healthScore;
            node.reliabilityScore = smooth(node.reliabilityScore, 1.0 - std::max(measuredTimeoutPressure, measuredRetryPressure * 0.65));
        } else if (NodeRegistry::generatesRequests(node.type)) {
            int outstanding = 0;
            double waitSum = 0.0;
            for (const auto& [id, request] : requests_) {
                (void)id;
                if (request.sourceNodeId != node.id) {
                    continue;
                }
                if (request.state == RequestState::Completed || request.state == RequestState::TimedOut) {
                    continue;
                }
                ++outstanding;
                waitSum += std::max(0.0, timeSeconds_ - request.creationTime);
            }

            const double expectedRecentDemand = std::max(1.0, node.recentGenerated);
            const double measuredQueuePressure = std::clamp(static_cast<double>(outstanding) / (expectedRecentDemand * 2.5), 0.0, 1.0);
            const double measuredLatencyPressure = outstanding > 0
                ? std::clamp((waitSum / outstanding) / std::max(0.25, scenario_.requestTimeoutSeconds), 0.0, 1.0)
                : 0.0;
            const double measuredStress = std::clamp(
                measuredLatencyPressure * 0.34
                    + measuredTimeoutPressure * 0.30
                    + measuredRetryPressure * 0.18
                    + measuredQueuePressure * 0.12
                    + node.propagatedPressure * 0.10
                    + node.propagatedInstability * 0.08,
                0.0,
                1.0);

            node.queuePressure = smooth(node.queuePressure, measuredQueuePressure);
            node.latencyPressure = smooth(node.latencyPressure, measuredLatencyPressure);
            node.timeoutPressure = smooth(node.timeoutPressure, measuredTimeoutPressure);
            node.retryPressure = smooth(node.retryPressure, measuredRetryPressure);
            node.stressScore = smooth(node.stressScore, measuredStress);
            node.experienceScore = smooth(node.experienceScore, 1.0 - measuredStress);
            node.healthScore = node.experienceScore;
            node.reliabilityScore = smooth(node.reliabilityScore, 1.0 - std::max(measuredTimeoutPressure, measuredRetryPressure * 0.75));
            node.currentUtilization = 0.0;
        } else {
            const double measuredStress = std::max({measuredTimeoutPressure, measuredRetryPressure, node.propagatedPressure, node.propagatedInstability});
            node.timeoutPressure = smooth(node.timeoutPressure, measuredTimeoutPressure);
            node.retryPressure = smooth(node.retryPressure, measuredRetryPressure);
            node.stressScore = smooth(node.stressScore, measuredStress);
            node.healthScore = smooth(node.healthScore, 1.0 - measuredStress);
            node.experienceScore = node.healthScore;
            node.reliabilityScore = smooth(node.reliabilityScore, 1.0 - measuredStress);
        }

        if (node.healthScore < 0.45 || node.timeoutPressure > 0.35) {
            node.health = HealthState::Failing;
        } else if (node.healthScore < 0.72 || node.stressScore > 0.45 || node.retryPressure > 0.18 || node.propagatedInstability > 0.50) {
            node.health = HealthState::Saturated;
        } else {
            node.health = HealthState::Healthy;
        }

        node.recentGenerated *= decay;
        node.recentCompleted *= decay;
        node.recentTimedOut *= decay;
        node.recentRetries *= decay;
    }
}

void Simulation::updateMetricsNodeStates()
{
    int apiQueueDepth = 0;
    int databaseQueueDepth = 0;
    double apiUtilization = 0.0;
    double databaseUtilization = 0.0;

    if (const auto apiId = firstNodeOfType(NodeType::ApiService)) {
        if (const Node* api = graph_.node(*apiId)) {
            apiQueueDepth = static_cast<int>(api->queue.size());
            apiUtilization = api->currentUtilization;
        }
    }

    if (const auto databaseId = firstNodeOfType(NodeType::Database)) {
        if (const Node* database = graph_.node(*databaseId)) {
            databaseQueueDepth = static_cast<int>(database->queue.size());
            databaseUtilization = database->currentUtilization;
        }
    }

    metrics_.setNodeStates(apiQueueDepth, apiUtilization, databaseQueueDepth, databaseUtilization);
}

