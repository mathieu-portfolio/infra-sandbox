#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>


void Simulation::generateClientRequests(double dt)
{
    double burstMultiplier = 1.0;
    const BurstScenario activeBurst = scenarioBurstOverride_.value_or(scenario_.bursts);
    if (burstModeEnabled_ || activeBurst.enabled) {
        const double phase = std::fmod(timeSeconds_, activeBurst.periodSeconds);
        if (phase < activeBurst.durationSeconds) {
            burstMultiplier = activeBurst.multiplier;
        }
    }

    for (auto& node : graph_.nodes()) {
        if (!NodeRegistry::generatesRequests(node.type)) {
            continue;
        }

        Link* link = selectClientIngressLink(node);
        if (link == nullptr) {
            continue;
        }

        const double evolutionMultiplier = trafficEvolutionMultiplierFor(node, burstMultiplier);
        node.generationAccumulator += node.requestRatePerSecond * scenarioTrafficMultiplier_ * localizedTrafficMultiplierFor(node) * burstMultiplier * evolutionMultiplier * dt;
        while (node.generationAccumulator >= 1.0) {
            createRequest(node, *link);
            node.generationAccumulator -= 1.0;
            link = selectClientIngressLink(node);
            if (link == nullptr) {
                break;
            }
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
    request.routeStage = RequestRouteStage::ToApi;
    request.creationTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;

    const int sequence = static_cast<int>(request.id % 100);
    const double lightweightShare = scenarioDatabaseHeavyShareOverride_
        ? 1.0 - *scenarioDatabaseHeavyShareOverride_
        : scenario_.requestTypes.lightweightShare;
    const int lightThreshold = static_cast<int>(lightweightShare * 100.0);
    request.type = sequence < lightThreshold ? RequestType::Lightweight : RequestType::DatabaseHeavy;
    const int cacheRoll = static_cast<int>((request.id * 37U) % 100U);
    request.cacheKey = static_cast<int>(request.id % std::max(1, scenario_.requestTypes.cacheKeySpace));
    request.cacheable = request.type == RequestType::DatabaseHeavy
        && cacheRoll < static_cast<int>(scenario_.requestTypes.databaseHeavyCacheableShare * 100.0);

    link.inFlightRequests.push_back(request.id);
    requests_.emplace(request.id, request);
    clientNode.recentGenerated += 1.0;
    metrics_.recordGenerated();
}

void Simulation::retryRequest(Request& request)
{
    Link* link = selectOutgoingLink(request.sourceNodeId, RequestRouteStage::ToApi);
    if (link == nullptr) {
        request.state = RequestState::TimedOut;
        request.completedTime = timeSeconds_;
        return;
    }

    request.targetNodeId = link->targetNodeId;
    request.currentNodeId = -1;
    request.currentLinkId = link->id;
    request.routeStage = RequestRouteStage::ToApi;
    request.state = RequestState::InTransit;
    request.creationTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
    request.transitProgress = 0.0;
    request.servedFromCache = false;
    link->inFlightRequests.push_back(request.id);
    if (Node* source = graph_.node(request.sourceNodeId)) {
        source->recentRetries += 1.0;
    }
    metrics_.recordRetry();
}

void Simulation::updateRetryWaits()
{
    for (auto& [id, request] : requests_) {
        (void)id;
        if (request.state == RequestState::RetryWaiting && timeSeconds_ >= request.retryDueTime) {
            retryRequest(request);
        }
    }
}

void Simulation::updateLinks(double dt)
{
    for (auto& link : graph_.links()) {
        if (!link.enabled) {
            continue;
        }
        std::vector<std::uint64_t> stillInFlight;
        stillInFlight.reserve(link.inFlightRequests.size());

        for (const auto requestId : link.inFlightRequests) {
            auto requestIt = requests_.find(requestId);
            if (requestIt == requests_.end()) {
                continue;
            }

            Request& request = requestIt->second;
            if (requestTimedOut(request)) {
                timeOutRequest(request);
                continue;
            }

            const double latency = std::max(0.01, link.baseLatencySeconds * scenarioLatencyMultiplier_);
            request.transitProgress += dt / latency;

            if (request.transitProgress >= 1.0) {
                if (Node* target = graph_.node(link.targetNodeId)) {
                    enqueueAtNode(request, *target);
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
        const double cascadeCapacityPenalty = std::clamp(1.0 - node.propagatedInstability * 0.35, 0.45, 1.0);
        const double availableProcessingBudget = std::max(0.0, node.processingCapacityPerSecond * cascadeCapacityPenalty * dt);
        double consumedProcessingBudget = 0.0;
        node.processingAccumulator += availableProcessingBudget;

        while (!node.queue.empty()) {
            const auto requestId = node.queue.front();
            auto requestIt = requests_.find(requestId);
            if (requestIt == requests_.end()) {
                node.queue.pop_front();
                continue;
            }

            Request& request = requestIt->second;
            const double cost = processingCost(request, node);

            // Model response/finalization work explicitly for API requests that
            // complete at the API during this processing pass. This keeps API
            // utilization visible for lightweight responses and cache hits even
            // though the client response leg is not represented as a separate
            // network hop in the topology.
            double completionCost = 0.0;
            if (node.type == NodeType::ApiService && request.routeStage == RequestRouteStage::ApiIngress) {
                const bool lightweightCompletesAtApi = request.type == RequestType::Lightweight;
                const bool cacheHitCompletesAtApi = request.type == RequestType::DatabaseHeavy
                    && request.cacheable
                    && cacheEnabled_
                    && cacheHit(request.cacheKey);
                if (lightweightCompletesAtApi || cacheHitCompletesAtApi) {
                    completionCost = scenario_.requestTypes.apiCostReturn;
                }
            }

            const double totalCost = cost + completionCost;
            if (node.processingAccumulator < totalCost) {
                break;
            }

            node.queue.pop_front();
            node.processingAccumulator -= totalCost;
            consumedProcessingBudget += totalCost;

            if (requestTimedOut(request)) {
                timeOutRequest(request);
                continue;
            }

            completeRequest(request, node);
        }

        (void)startedWithQueueDepth;

        // Utilization is measured from work consumed over a rolling window, not
        // from queue depth. Queue depth is reported separately as queuePressure.
        constexpr double kUtilizationWindowSeconds = 3.0;
        const double decay = std::exp(-std::max(0.0, dt) / kUtilizationWindowSeconds);
        node.recentWorkConsumed = node.recentWorkConsumed * decay + consumedProcessingBudget;
        node.recentWorkCapacity = node.recentWorkCapacity * decay + availableProcessingBudget;
        node.currentUtilization = node.recentWorkCapacity > 0.0
            ? std::clamp(node.recentWorkConsumed / node.recentWorkCapacity, 0.0, 1.0)
            : 0.0;
        node.backlogPressure = std::clamp(static_cast<double>(node.queue.size()) / std::max(1.0, node.processingCapacityPerSecond * 2.0), 0.0, 1.0);
        node.recentJobsConsumed = node.recentJobsConsumed * decay + static_cast<double>(consumedProcessingBudget > 0.0 ? 1.0 : 0.0);

        const double utilization = node.currentUtilization;
        node.computePressure = std::clamp(utilization * std::max(0.0, node.computeWeight), 0.0, 1.0);
        node.memoryPressure = std::clamp((utilization * 0.7 + node.backlogPressure * 0.3) * std::max(0.0, node.memoryWeight), 0.0, 1.0);
        node.storagePressure = std::clamp((utilization * 0.5 + node.backlogPressure * 0.5) * std::max(0.0, node.storageWeight), 0.0, 1.0);
        node.networkPressure = std::clamp(utilization * std::max(0.0, node.networkWeight), 0.0, 1.0);
    }
}

void Simulation::updateTimeouts(Node& node)
{
    std::deque<std::uint64_t> retained;
    double waitSum = 0.0;
    int waitSamples = 0;

    while (!node.queue.empty()) {
        const auto requestId = node.queue.front();
        node.queue.pop_front();

        auto requestIt = requests_.find(requestId);
        if (requestIt == requests_.end()) {
            continue;
        }

        Request& request = requestIt->second;
        if (requestTimedOut(request)) {
            timeOutRequest(request);
        } else {
            waitSum += timeSeconds_ - request.stateEnteredTime;
            ++waitSamples;
            retained.push_back(requestId);
        }
    }

    node.averageQueueWaitSeconds = waitSamples > 0 ? waitSum / waitSamples : 0.0;
    node.queue = std::move(retained);
}

void Simulation::enqueueAtNode(Request& request, Node& node)
{
    request.currentNodeId = node.id;
    request.currentLinkId = -1;
    request.targetNodeId = node.id;
    request.state = RequestState::Queued;
    request.stateEnteredTime = timeSeconds_;
    request.transitProgress = 1.0;

    if (node.type == NodeType::ApiService && request.routeStage == RequestRouteStage::ToApi) {
        request.routeStage = RequestRouteStage::ApiIngress;
    } else if (node.type == NodeType::Database && request.routeStage == RequestRouteStage::ToDatabase) {
        request.routeStage = RequestRouteStage::DatabaseWork;
    } else if (node.type == NodeType::ApiService && request.routeStage == RequestRouteStage::BackToApi) {
        request.routeStage = RequestRouteStage::ApiReturn;
    }

    node.queue.push_back(request.id);
}

void Simulation::completeRequest(Request& request, Node& node)
{
    if (node.type == NodeType::ApiService) {
        routeFromApi(request);
        return;
    }

    if (node.type == NodeType::Database) {
        routeFromDatabase(request);
        return;
    }

    if (node.type == NodeType::ReadReplica) {
        routeFromDatabase(request);
        return;
    }

    if (node.type == NodeType::QueueBroker) {
        if (const auto workerId = firstNodeOfType(NodeType::Worker)) {
            if (Link* link = linkBetween(node.id, *workerId)) {
                routeToLink(request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }

        if (const auto databaseId = firstNodeOfType(NodeType::Database)) {
            if (Link* link = linkBetween(node.id, *databaseId)) {
                routeToLink(request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }
    }

    if (node.type == NodeType::Worker || node.type == NodeType::Cache) {
        const auto databaseId = firstNodeOfType(NodeType::Database);
        if (databaseId) {
            if (Link* link = linkBetween(node.id, *databaseId)) {
                routeToLink(request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }
    }

    request.state = RequestState::Completed;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
    if (Node* source = graph_.node(request.sourceNodeId)) {
        source->recentCompleted += 1.0;
    }
    metrics_.recordProcessed(timeSeconds_ - request.creationTime);
}

void Simulation::timeOutRequest(Request& request, Node&)
{
    timeOutRequest(request);
}

void Simulation::timeOutRequest(Request& request)
{
    if (scenario_.retries.enabled && request.retryCount < scenario_.retries.maxRetries) {
        ++request.retryCount;
        request.state = RequestState::RetryWaiting;
        request.currentLinkId = -1;
        request.completedTime = timeSeconds_;
        const Node* source = graph_.node(request.sourceNodeId);
        const double localizedRetryDelayMultiplier = source != nullptr ? localizedRetryDelayMultiplierFor(*source) : 1.0;
        const double evolutionRetryDelayMultiplier = source != nullptr ? retryEvolutionDelayMultiplierFor(*source) : 1.0;
        request.retryDueTime = timeSeconds_ + scenario_.retries.retryDelaySeconds * scenarioRetryDelayMultiplier_ * localizedRetryDelayMultiplier * evolutionRetryDelayMultiplier;
        request.stateEnteredTime = timeSeconds_;
        if (Node* mutableSource = graph_.node(request.sourceNodeId)) {
            mutableSource->recentTimedOut += 1.0;
        }
        metrics_.recordTimedOut(timeSeconds_ - request.creationTime);
        return;
    }

    request.state = RequestState::TimedOut;
    request.currentLinkId = -1;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
    if (Node* source = graph_.node(request.sourceNodeId)) {
        source->recentTimedOut += 1.0;
    }
    metrics_.recordTimedOut(timeSeconds_ - request.creationTime);
}

void Simulation::routeFromApi(Request& request)
{
    if (request.routeStage == RequestRouteStage::ApiReturn || request.type == RequestType::Lightweight) {
        if (request.type == RequestType::DatabaseHeavy && request.cacheable && cacheEnabled_) {
            storeCache(request.cacheKey);
        }
        request.state = RequestState::Completed;
        request.completedTime = timeSeconds_;
        request.stateEnteredTime = timeSeconds_;
        if (Node* source = graph_.node(request.sourceNodeId)) {
            source->recentCompleted += 1.0;
        }
        metrics_.recordProcessed(timeSeconds_ - request.creationTime);
        return;
    }

    if (request.type == RequestType::DatabaseHeavy) {
        if (request.cacheable && cacheEnabled_) {
            const bool hit = cacheHit(request.cacheKey);
            metrics_.recordCacheLookup(hit);
            if (hit) {
                request.servedFromCache = true;
                request.state = RequestState::Completed;
                request.completedTime = timeSeconds_;
                request.stateEnteredTime = timeSeconds_;
                if (Node* source = graph_.node(request.sourceNodeId)) {
                    source->recentCompleted += 1.0;
                }
                metrics_.recordProcessed(timeSeconds_ - request.creationTime);
                return;
            }
        }

        if (request.cacheable) {
            const auto replicaId = firstNodeOfType(NodeType::ReadReplica);
            if (replicaId) {
                if (Link* link = linkBetween(request.currentNodeId, *replicaId)) {
                    routeToLink(request, *link, RequestRouteStage::ToDatabase);
                    return;
                }
            }
        }

        const auto databaseId = firstNodeOfType(NodeType::Database);
        if (databaseId) {
            if (Link* link = linkBetween(request.currentNodeId, *databaseId)) {
                routeToLink(request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }

        if (Link* link = selectOutgoingLink(request.currentNodeId, RequestRouteStage::ToDatabase)) {
            routeToLink(request, *link, RequestRouteStage::ToDatabase);
            return;
        }
    }

    request.state = RequestState::Completed;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
    if (Node* source = graph_.node(request.sourceNodeId)) {
        source->recentCompleted += 1.0;
    }
    metrics_.recordProcessed(timeSeconds_ - request.creationTime);
}

void Simulation::routeFromDatabase(Request& request)
{
    const auto apiId = firstNodeOfType(NodeType::ApiService);
    if (apiId) {
        if (Link* link = linkBetween(request.currentNodeId, *apiId)) {
            routeToLink(request, *link, RequestRouteStage::BackToApi);
            return;
        }
    }

    request.state = RequestState::Completed;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
    if (Node* source = graph_.node(request.sourceNodeId)) {
        source->recentCompleted += 1.0;
    }
    metrics_.recordProcessed(timeSeconds_ - request.creationTime);
}

void Simulation::routeToLink(Request& request, Link& link, RequestRouteStage nextStage)
{
    request.currentLinkId = link.id;
    request.targetNodeId = link.targetNodeId;
    request.routeStage = nextStage;
    request.state = RequestState::InTransit;
    request.stateEnteredTime = timeSeconds_;
    request.transitProgress = 0.0;
    link.inFlightRequests.push_back(request.id);
}

Link* Simulation::selectOutgoingLink(int sourceNodeId, RequestRouteStage routeStage)
{
    std::vector<Link*> candidates;
    for (auto& link : graph_.links()) {
        if (link.enabled && link.sourceNodeId == sourceNodeId) {
            candidates.push_back(&link);
        }
    }
    if (candidates.empty()) {
        return nullptr;
    }

    const auto& evolution = scenario_.trafficProfile.evolution;
    if (!evolution.enabled || (evolution.reroutePressureSensitivity <= 0.0 && evolution.rerouteLatencySensitivity <= 0.0)) {
        return candidates.front();
    }

    Link* selected = candidates.front();
    double bestScore = std::numeric_limits<double>::max();
    for (Link* link : candidates) {
        const Node* target = graph_.node(link->targetNodeId);
        const double targetPressure = target != nullptr
            ? std::max({target->queuePressure, target->latencyPressure, target->retryPressure, target->propagatedInstability})
            : 0.0;
        const double stageBias = routeStage == RequestRouteStage::ToApi ? 0.75 : 1.0;
        const double score =
            static_cast<double>(link->inFlightRequests.size()) * 0.05
            + evolution.reroutePressureSensitivity * targetPressure * stageBias
            + evolution.rerouteLatencySensitivity * link->baseLatencySeconds;
        if (score < bestScore) {
            bestScore = score;
            selected = link;
        }
    }
    return selected;
}

Link* Simulation::selectClientIngressLink(const Node& clientNode)
{
    Link* selected = selectOutgoingLink(clientNode.id, RequestRouteStage::ToApi);
    const auto& evolution = scenario_.trafficProfile.evolution;
    if (selected == nullptr || !evolution.enabled || evolution.migrationSensitivity <= 0.0) {
        return selected;
    }

    const Node* target = graph_.node(selected->targetNodeId);
    if (target == nullptr) {
        return selected;
    }
    const double clientStress = std::clamp(1.0 - clientNode.experienceScore + target->propagatedInstability, 0.0, 1.0);
    if (clientStress <= 0.0) {
        return selected;
    }

    Link* migrated = selected;
    double bestScore = std::numeric_limits<double>::max();
    for (auto& link : graph_.links()) {
        if (!link.enabled || link.sourceNodeId != clientNode.id) {
            continue;
        }
        const Node* candidateTarget = graph_.node(link.targetNodeId);
        const double candidatePressure = candidateTarget != nullptr
            ? std::max(candidateTarget->queuePressure, candidateTarget->latencyPressure)
            : 0.0;
        const double score = candidatePressure + link.baseLatencySeconds * 0.35 + static_cast<double>(link.inFlightRequests.size()) * 0.03;
        if (score < bestScore) {
            bestScore = score;
            migrated = &link;
        }
    }

    const double migrationGate = std::clamp(evolution.migrationSensitivity * clientStress, 0.0, 1.0);
    const int deterministicRoll = static_cast<int>((nextRequestId_ * 53U + static_cast<std::uint64_t>(clientNode.id) * 17U) % 100U);
    return deterministicRoll < static_cast<int>(migrationGate * 100.0) ? migrated : selected;
}

double Simulation::trafficEvolutionMultiplierFor(const Node& node, double burstMultiplier) const
{
    const auto& evolution = scenario_.trafficProfile.evolution;
    if (!evolution.enabled) {
        return 1.0;
    }

    const double failureShare = node.recentGenerated > 0.1
        ? std::clamp((node.recentTimedOut + node.recentRetries * 0.5) / node.recentGenerated, 0.0, 1.0)
        : 0.0;
    const double pressureBoost = 1.0 + evolution.pressureSensitivity * std::max(node.latencyPressure, node.retryPressure);
    const double churnLoss = 1.0 - evolution.churnSensitivity * std::max(0.0, 1.0 - node.experienceScore);
    const double burstBoost = burstMultiplier > 1.0
        ? 1.0 + evolution.burstAmplification * (burstMultiplier - 1.0)
        : 1.0;
    const double failureChurn = 1.0 - evolution.churnSensitivity * 0.5 * failureShare;
    return std::clamp(pressureBoost * churnLoss * burstBoost * failureChurn, 0.2, 3.0);
}

double Simulation::retryEvolutionDelayMultiplierFor(const Node& node) const
{
    const auto& evolution = scenario_.trafficProfile.evolution;
    if (!evolution.enabled || evolution.dynamicRetrySensitivity <= 0.0) {
        return 1.0;
    }

    const double sourcePressure = std::clamp(std::max(node.retryPressure, node.timeoutPressure), 0.0, 1.0);
    return std::clamp(1.0 - evolution.dynamicRetrySensitivity * sourcePressure, 0.25, 1.0);
}

Link* Simulation::linkBetween(int sourceNodeId, int targetNodeId)
{
    for (auto& link : graph_.links()) {
        if (link.enabled && link.sourceNodeId == sourceNodeId && link.targetNodeId == targetNodeId) {
            return &link;
        }
    }
    return nullptr;
}

std::optional<int> Simulation::firstNodeOfType(NodeType type) const
{
    for (const auto& node : graph_.nodes()) {
        if (node.type == type) {
            return node.id;
        }
    }
    return std::nullopt;
}

double Simulation::processingCost(const Request& request, const Node& node) const
{
    if (node.type == NodeType::Database || node.type == NodeType::ReadReplica) {
        return scenario_.requestTypes.databaseCostHeavy;
    }

    if (node.type == NodeType::QueueBroker) {
        return 0.2;
    }

    if (node.type == NodeType::Worker || node.type == NodeType::BatchProcessor || node.type == NodeType::StreamProcessor) {
        return request.type == RequestType::DatabaseHeavy ? 1.1 : 0.55;
    }

    if (node.type == NodeType::Cache) {
        return 0.25;
    }

    if (request.routeStage == RequestRouteStage::ApiReturn) {
        return scenario_.requestTypes.apiCostReturn;
    }

    return request.type == RequestType::Lightweight
        ? scenario_.requestTypes.apiCostLightweight
        : scenario_.requestTypes.apiCostDatabaseHeavy;
}

bool Simulation::requestTimedOut(const Request& request) const
{
    return timeSeconds_ - request.creationTime > scenario_.requestTimeoutSeconds;
}

bool Simulation::cacheHit(int key)
{
    expireCacheEntries();
    return std::any_of(cacheEntries_.begin(), cacheEntries_.end(), [key](const CacheEntry& entry) {
        return entry.key == key;
    });
}

void Simulation::storeCache(int key)
{
    if (!cacheEnabled_) {
        return;
    }

    cacheEntries_.erase(
        std::remove_if(cacheEntries_.begin(), cacheEntries_.end(), [key](const CacheEntry& entry) {
            return entry.key == key;
        }),
        cacheEntries_.end());

    while (static_cast<int>(cacheEntries_.size()) >= scenario_.cache.maxEntries) {
        cacheEntries_.pop_front();
    }

    cacheEntries_.push_back({key, timeSeconds_ + scenario_.cache.ttlSeconds});
}

void Simulation::expireCacheEntries()
{
    cacheEntries_.erase(
        std::remove_if(cacheEntries_.begin(), cacheEntries_.end(), [this](const CacheEntry& entry) {
            return entry.expiresAt <= timeSeconds_;
        }),
        cacheEntries_.end());
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
