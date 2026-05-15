#include "simulation/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

Simulation::Simulation(const ScenarioDefinition& scenario)
{
    buildFromScenario(scenario);
}

void Simulation::update(double dt)
{
    timeSeconds_ += dt;
    generateClientRequests(dt);
    updateLinks(dt);
    updateProcessors(dt);
    updateNodeHealth();

    if (const auto backendId = graph_.firstProcessorNodeId()) {
        if (const Node* backend = graph_.node(*backendId)) {
            metrics_.setBackendState(static_cast<int>(backend->queue.size()), backend->currentUtilization);
        }
    }

    metrics_.update(dt);
    pruneOldRequests();
}

void Simulation::adjustClientRequestRates(double deltaPerSecond)
{
    for (auto& node : graph_.nodes()) {
        if (node.type == NodeType::ClientCluster) {
            node.requestRatePerSecond = std::max(0.0, node.requestRatePerSecond + deltaPerSecond);
            node.baseRequestRatePerSecond = node.requestRatePerSecond;
        }
    }
}

void Simulation::scaleProcessingCapacity(double multiplier)
{
    for (auto& node : graph_.nodes()) {
        if (node.isProcessor()) {
            node.processingCapacityPerSecond = std::max(0.1, node.processingCapacityPerSecond * multiplier);
        }
    }
}

void Simulation::applyCachePlaceholder()
{
    cacheEfficiency_ = std::min(0.6, cacheEfficiency_ + 0.15);
}

void Simulation::resetProcessingCapacity()
{
    cacheEfficiency_ = 0.0;
    for (auto& node : graph_.nodes()) {
        if (node.isProcessor()) {
            node.processingCapacityPerSecond = node.baseProcessingCapacityPerSecond;
        }
    }
}

const InfrastructureGraph& Simulation::graph() const
{
    return graph_;
}

const std::unordered_map<std::uint64_t, Request>& Simulation::requests() const
{
    return requests_;
}

const MetricsSnapshot& Simulation::metrics() const
{
    return metrics_.snapshot();
}

double Simulation::timeSeconds() const
{
    return timeSeconds_;
}

double Simulation::cacheEfficiency() const
{
    return cacheEfficiency_;
}

void Simulation::buildFromScenario(const ScenarioDefinition& scenario)
{
    graph_ = InfrastructureGraph{};
    requests_.clear();
    metrics_.reset();
    nextRequestId_ = 1;
    timeSeconds_ = 0.0;
    cacheEfficiency_ = 0.0;

    for (const auto& nodeScenario : scenario.nodes) {
        Node node;
        node.name = nodeScenario.name;
        node.type = nodeScenario.type;
        node.position = nodeScenario.position;
        node.requestRatePerSecond = nodeScenario.requestRatePerSecond;
        node.baseRequestRatePerSecond = nodeScenario.requestRatePerSecond;
        node.processingCapacityPerSecond = nodeScenario.processingCapacityPerSecond;
        node.baseProcessingCapacityPerSecond = nodeScenario.processingCapacityPerSecond;
        node.timeoutSeconds = nodeScenario.timeoutSeconds;
        graph_.addNode(std::move(node));
    }

    for (const auto& linkScenario : scenario.links) {
        Link link;
        link.sourceNodeId = linkScenario.sourceNode;
        link.targetNodeId = linkScenario.targetNode;
        link.baseLatencySeconds = linkScenario.baseLatencySeconds;
        link.bandwidthPerSecond = linkScenario.bandwidthPerSecond;
        graph_.addLink(std::move(link));
    }
}

void Simulation::generateClientRequests(double dt)
{
    for (auto& node : graph_.nodes()) {
        if (node.type != NodeType::ClientCluster) {
            continue;
        }

        Link* link = graph_.firstOutgoingLink(node.id);
        if (link == nullptr) {
            continue;
        }

        const double effectiveRate = node.requestRatePerSecond * (1.0 - cacheEfficiency_);
        node.generationAccumulator += effectiveRate * dt;
        while (node.generationAccumulator >= 1.0) {
            createRequest(node, *link);
            node.generationAccumulator -= 1.0;
        }
    }
}

void Simulation::createRequest(Node& clientNode, Link& link)
{
    Request request;
    request.id = nextRequestId_++;
    request.sourceNodeId = clientNode.id;
    request.targetNodeId = link.targetNodeId;
    request.currentLinkId = link.id;
    request.state = RequestState::InTransit;
    request.creationTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;

    link.inFlightRequests.push_back(request.id);
    requests_.emplace(request.id, request);
    metrics_.recordGenerated();
}

void Simulation::updateLinks(double dt)
{
    for (auto& link : graph_.links()) {
        std::vector<std::uint64_t> stillInFlight;
        stillInFlight.reserve(link.inFlightRequests.size());

        for (const auto requestId : link.inFlightRequests) {
            auto requestIt = requests_.find(requestId);
            if (requestIt == requests_.end()) {
                continue;
            }

            Request& request = requestIt->second;
            const double latency = std::max(0.01, link.baseLatencySeconds);
            request.transitProgress += dt / latency;

            if (request.transitProgress >= 1.0) {
                if (Node* target = graph_.node(link.targetNodeId)) {
                    request.currentNodeId = target->id;
                    request.currentLinkId = -1;
                    request.targetNodeId = target->id;
                    request.state = RequestState::Queued;
                    request.stateEnteredTime = timeSeconds_;
                    request.transitProgress = 1.0;
                    target->queue.push_back(request.id);
                }
            } else {
                stillInFlight.push_back(requestId);
            }
        }

        link.inFlightRequests = std::move(stillInFlight);
    }
}

void Simulation::updateProcessors(double dt)
{
    for (auto& node : graph_.nodes()) {
        if (!node.isProcessor()) {
            continue;
        }

        updateTimeouts(node);

        const auto startedWithQueueDepth = node.queue.size();
        node.processingAccumulator += node.processingCapacityPerSecond * dt;

        while (node.processingAccumulator >= 1.0 && !node.queue.empty()) {
            const auto requestId = node.queue.front();
            node.queue.pop_front();
            node.processingAccumulator -= 1.0;

            auto requestIt = requests_.find(requestId);
            if (requestIt == requests_.end()) {
                continue;
            }

            Request& request = requestIt->second;
            if (timeSeconds_ - request.creationTime > node.timeoutSeconds) {
                timeOutRequest(request, node);
                continue;
            }

            completeRequest(request, node);
        }

        const double pressure = node.processingCapacityPerSecond > 0.0
            ? static_cast<double>(startedWithQueueDepth) / node.processingCapacityPerSecond
            : 1.0;
        node.currentUtilization = std::clamp(pressure, 0.0, 1.0);
    }
}

void Simulation::updateTimeouts(Node& node)
{
    std::deque<std::uint64_t> retained;
    while (!node.queue.empty()) {
        const auto requestId = node.queue.front();
        node.queue.pop_front();

        auto requestIt = requests_.find(requestId);
        if (requestIt == requests_.end()) {
            continue;
        }

        Request& request = requestIt->second;
        if (timeSeconds_ - request.creationTime > node.timeoutSeconds) {
            timeOutRequest(request, node);
        } else {
            retained.push_back(requestId);
        }
    }

    node.queue = std::move(retained);
}

void Simulation::completeRequest(Request& request, Node&)
{
    if (const Link* outgoing = graph_.firstOutgoingLink(request.currentNodeId)) {
        request.currentLinkId = outgoing->id;
        request.targetNodeId = outgoing->targetNodeId;
        request.state = RequestState::InTransit;
        request.stateEnteredTime = timeSeconds_;
        request.transitProgress = 0.0;

        if (Link* mutableOutgoing = graph_.link(outgoing->id)) {
            mutableOutgoing->inFlightRequests.push_back(request.id);
        }
        return;
    }

    request.state = RequestState::Completed;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
    metrics_.recordProcessed(timeSeconds_ - request.creationTime);
}

void Simulation::timeOutRequest(Request& request, Node&)
{
    request.state = RequestState::TimedOut;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
    metrics_.recordTimedOut(timeSeconds_ - request.creationTime);
}

void Simulation::updateNodeHealth()
{
    for (auto& node : graph_.nodes()) {
        if (!node.isProcessor()) {
            node.health = HealthState::Healthy;
            continue;
        }

        if (node.queue.size() > static_cast<std::size_t>(node.processingCapacityPerSecond * node.timeoutSeconds * 0.75)) {
            node.health = HealthState::Failing;
        } else if (node.currentUtilization > 0.85 || node.queue.size() > static_cast<std::size_t>(node.processingCapacityPerSecond)) {
            node.health = HealthState::Saturated;
        } else {
            node.health = HealthState::Healthy;
        }
    }
}

void Simulation::pruneOldRequests()
{
    for (auto it = requests_.begin(); it != requests_.end();) {
        const Request& request = it->second;
        const bool terminal = request.state == RequestState::Completed || request.state == RequestState::TimedOut;
        if (terminal && timeSeconds_ - request.completedTime > 3.0) {
            it = requests_.erase(it);
        } else {
            ++it;
        }
    }
}
