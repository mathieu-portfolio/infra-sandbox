#include "simulation/systems/SimulationRequestFlowSystem.hpp"
#include "simulation/systems/SimulationModifierSystem.hpp"

#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>


void SimulationRequestFlowSystem::generateClientRequests(Simulation& simulation, double dt)
{
    double burstMultiplier = 1.0;
    const BurstScenario activeBurst = simulation.scenarioBurstOverride_.value_or(simulation.scenario_.bursts);
    if (simulation.burstModeEnabled_ || activeBurst.enabled) {
        const double phase = std::fmod(simulation.timeSeconds_, activeBurst.periodSeconds);
        if (phase < activeBurst.durationSeconds) {
            burstMultiplier = activeBurst.multiplier;
        }
    }

    for (auto& node : simulation.graph_.nodes()) {
        if (!NodeRegistry::generatesRequests(node.type)) {
            continue;
        }

        Link* link = selectClientIngressLink(simulation, node);
        if (link == nullptr) {
            continue;
        }

        const double evolutionMultiplier = trafficEvolutionMultiplierFor(simulation, node, burstMultiplier);
        node.generationAccumulator += node.requestRatePerSecond * simulation.scenarioTrafficMultiplier_ * SimulationModifierSystem::localizedTrafficMultiplierFor(simulation, node) * burstMultiplier * evolutionMultiplier * dt;
        while (node.generationAccumulator >= 1.0) {
            createRequest(simulation, node, *link);
            node.generationAccumulator -= 1.0;
            link = selectClientIngressLink(simulation, node);
            if (link == nullptr) {
                break;
            }
        }
    }
}

void SimulationRequestFlowSystem::createRequest(Simulation& simulation, Node& clientNode, Link& link)
{
    Request request;
    request.id = simulation.nextRequestId_++;
    request.sourceNodeId = clientNode.id;
    request.targetNodeId = link.targetNodeId;
    request.currentLinkId = link.id;
    request.state = RequestState::InTransit;
    request.routeStage = RequestRouteStage::ToApi;
    request.creationTime = simulation.timeSeconds_;
    request.stateEnteredTime = simulation.timeSeconds_;

    const int sequence = static_cast<int>(request.id % 100);
    const double lightweightShare = simulation.scenarioDatabaseHeavyShareOverride_
        ? 1.0 - *simulation.scenarioDatabaseHeavyShareOverride_
        : simulation.scenario_.requestTypes.lightweightShare;
    const int lightThreshold = static_cast<int>(lightweightShare * 100.0);
    request.type = sequence < lightThreshold ? RequestType::Lightweight : RequestType::DatabaseHeavy;
    const int cacheRoll = static_cast<int>((request.id * 37U) % 100U);
    request.cacheKey = static_cast<int>(request.id % std::max(1, simulation.scenario_.requestTypes.cacheKeySpace));
    request.cacheable = request.type == RequestType::DatabaseHeavy
        && cacheRoll < static_cast<int>(simulation.scenario_.requestTypes.databaseHeavyCacheableShare * 100.0);

    link.inFlightRequests.push_back(request.id);
    simulation.requests_.emplace(request.id, request);
    clientNode.recentGenerated += 1.0;
    simulation.metrics_.recordGenerated();
}

void SimulationRequestFlowSystem::retryRequest(Simulation& simulation, Request& request)
{
    Link* link = selectOutgoingLink(simulation, request.sourceNodeId, RequestRouteStage::ToApi);
    if (link == nullptr) {
        request.state = RequestState::TimedOut;
        request.completedTime = simulation.timeSeconds_;
        return;
    }

    request.targetNodeId = link->targetNodeId;
    request.currentNodeId = -1;
    request.currentLinkId = link->id;
    request.routeStage = RequestRouteStage::ToApi;
    request.state = RequestState::InTransit;
    request.creationTime = simulation.timeSeconds_;
    request.stateEnteredTime = simulation.timeSeconds_;
    request.transitProgress = 0.0;
    request.servedFromCache = false;
    link->inFlightRequests.push_back(request.id);
    if (Node* source = simulation.graph_.node(request.sourceNodeId)) {
        source->recentRetries += 1.0;
    }
    simulation.metrics_.recordRetry();
}

void SimulationRequestFlowSystem::updateRetryWaits(Simulation& simulation)
{
    for (auto& [id, request] : simulation.requests_) {
        (void)id;
        if (request.state == RequestState::RetryWaiting && simulation.timeSeconds_ >= request.retryDueTime) {
            retryRequest(simulation, request);
        }
    }
}

void SimulationRequestFlowSystem::updateLinks(Simulation& simulation, double dt)
{
    for (auto& link : simulation.graph_.links()) {
        if (!link.enabled) {
            continue;
        }
        std::vector<std::uint64_t> stillInFlight;
        stillInFlight.reserve(link.inFlightRequests.size());

        for (const auto requestId : link.inFlightRequests) {
            auto requestIt = simulation.requests_.find(requestId);
            if (requestIt == simulation.requests_.end()) {
                continue;
            }

            Request& request = requestIt->second;
            if (requestTimedOut(simulation, request)) {
                timeOutRequest(simulation, request);
                continue;
            }

            const double latency = std::max(0.01, link.baseLatencySeconds * simulation.scenarioLatencyMultiplier_);
            request.transitProgress += dt / latency;

            if (request.transitProgress >= 1.0) {
                if (Node* target = simulation.graph_.node(link.targetNodeId)) {
                    enqueueAtNode(simulation, request, *target);
                }
            } else {
                stillInFlight.push_back(requestId);
            }
        }

        link.inFlightRequests = std::move(stillInFlight);
    }
}

void SimulationRequestFlowSystem::updateProcessors(Simulation& simulation, double dt)
{
    for (auto& node : simulation.graph_.nodes()) {
        if (!node.isProcessor()) {
            continue;
        }

        updateTimeouts(simulation, node);

        const auto startedWithQueueDepth = node.queue.size();
        const double cascadeCapacityPenalty = std::clamp(1.0 - node.propagatedInstability * 0.35, 0.45, 1.0);
        const double availableProcessingBudget = std::max(0.0, node.processingCapacityPerSecond * cascadeCapacityPenalty * dt);
        double consumedProcessingBudget = 0.0;
        node.processingAccumulator += availableProcessingBudget;

        while (!node.queue.empty()) {
            const auto requestId = node.queue.front();
            auto requestIt = simulation.requests_.find(requestId);
            if (requestIt == simulation.requests_.end()) {
                node.queue.pop_front();
                continue;
            }

            Request& request = requestIt->second;
            const double cost = processingCost(simulation, request, node);

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
                    && simulation.cacheEnabled_
                    && cacheHit(simulation, request.cacheKey);
                if (lightweightCompletesAtApi || cacheHitCompletesAtApi) {
                    completionCost = simulation.scenario_.requestTypes.apiCostReturn;
                }
            }

            const double totalCost = cost + completionCost;
            if (node.processingAccumulator < totalCost) {
                break;
            }

            node.queue.pop_front();
            node.processingAccumulator -= totalCost;
            consumedProcessingBudget += totalCost;

            if (requestTimedOut(simulation, request)) {
                timeOutRequest(simulation, request);
                continue;
            }

            completeRequest(simulation, request, node);
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

void SimulationRequestFlowSystem::updateTimeouts(Simulation& simulation, Node& node)
{
    std::deque<std::uint64_t> retained;
    double waitSum = 0.0;
    int waitSamples = 0;

    while (!node.queue.empty()) {
        const auto requestId = node.queue.front();
        node.queue.pop_front();

        auto requestIt = simulation.requests_.find(requestId);
        if (requestIt == simulation.requests_.end()) {
            continue;
        }

        Request& request = requestIt->second;
        if (requestTimedOut(simulation, request)) {
            timeOutRequest(simulation, request);
        } else {
            waitSum += simulation.timeSeconds_ - request.stateEnteredTime;
            ++waitSamples;
            retained.push_back(requestId);
        }
    }

    node.averageQueueWaitSeconds = waitSamples > 0 ? waitSum / waitSamples : 0.0;
    node.queue = std::move(retained);
}

void SimulationRequestFlowSystem::enqueueAtNode(Simulation& simulation, Request& request, Node& node)
{
    request.currentNodeId = node.id;
    request.currentLinkId = -1;
    request.targetNodeId = node.id;
    request.state = RequestState::Queued;
    request.stateEnteredTime = simulation.timeSeconds_;
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

void SimulationRequestFlowSystem::completeRequest(Simulation& simulation, Request& request, Node& node)
{
    if (node.type == NodeType::ApiService) {
        routeFromApi(simulation, request);
        return;
    }

    if (node.type == NodeType::Database) {
        routeFromDatabase(simulation, request);
        return;
    }

    if (node.type == NodeType::ReadReplica) {
        routeFromDatabase(simulation, request);
        return;
    }

    if (node.type == NodeType::QueueBroker) {
        if (const auto workerId = firstNodeOfType(simulation, NodeType::Worker)) {
            if (Link* link = linkBetween(simulation, node.id, *workerId)) {
                routeToLink(simulation, request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }

        if (const auto databaseId = firstNodeOfType(simulation, NodeType::Database)) {
            if (Link* link = linkBetween(simulation, node.id, *databaseId)) {
                routeToLink(simulation, request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }
    }

    if (node.type == NodeType::Worker || node.type == NodeType::Cache) {
        const auto databaseId = firstNodeOfType(simulation, NodeType::Database);
        if (databaseId) {
            if (Link* link = linkBetween(simulation, node.id, *databaseId)) {
                routeToLink(simulation, request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }
    }

    request.state = RequestState::Completed;
    request.completedTime = simulation.timeSeconds_;
    request.stateEnteredTime = simulation.timeSeconds_;
    if (Node* source = simulation.graph_.node(request.sourceNodeId)) {
        source->recentCompleted += 1.0;
    }
    simulation.metrics_.recordProcessed(simulation.timeSeconds_ - request.creationTime);
}

void SimulationRequestFlowSystem::timeOutRequest(Simulation& simulation, Request& request, Node&)
{
    timeOutRequest(simulation, request);
}

void SimulationRequestFlowSystem::timeOutRequest(Simulation& simulation, Request& request)
{
    if (simulation.scenario_.retries.enabled && request.retryCount < simulation.scenario_.retries.maxRetries) {
        ++request.retryCount;
        request.state = RequestState::RetryWaiting;
        request.currentLinkId = -1;
        request.completedTime = simulation.timeSeconds_;
        const Node* source = simulation.graph_.node(request.sourceNodeId);
        const double localizedRetryDelayMultiplier = source != nullptr ? SimulationModifierSystem::localizedRetryDelayMultiplierFor(simulation, *source) : 1.0;
        const double evolutionRetryDelayMultiplier = source != nullptr ? retryEvolutionDelayMultiplierFor(simulation, *source) : 1.0;
        request.retryDueTime = simulation.timeSeconds_ + simulation.scenario_.retries.retryDelaySeconds * simulation.scenarioRetryDelayMultiplier_ * localizedRetryDelayMultiplier * evolutionRetryDelayMultiplier;
        request.stateEnteredTime = simulation.timeSeconds_;
        if (Node* mutableSource = simulation.graph_.node(request.sourceNodeId)) {
            mutableSource->recentTimedOut += 1.0;
        }
        simulation.metrics_.recordTimedOut(simulation.timeSeconds_ - request.creationTime);
        return;
    }

    request.state = RequestState::TimedOut;
    request.currentLinkId = -1;
    request.completedTime = simulation.timeSeconds_;
    request.stateEnteredTime = simulation.timeSeconds_;
    if (Node* source = simulation.graph_.node(request.sourceNodeId)) {
        source->recentTimedOut += 1.0;
    }
    simulation.metrics_.recordTimedOut(simulation.timeSeconds_ - request.creationTime);
}

void SimulationRequestFlowSystem::routeFromApi(Simulation& simulation, Request& request)
{
    if (request.routeStage == RequestRouteStage::ApiReturn || request.type == RequestType::Lightweight) {
        if (request.type == RequestType::DatabaseHeavy && request.cacheable && simulation.cacheEnabled_) {
            storeCache(simulation, request.cacheKey);
        }
        request.state = RequestState::Completed;
        request.completedTime = simulation.timeSeconds_;
        request.stateEnteredTime = simulation.timeSeconds_;
        if (Node* source = simulation.graph_.node(request.sourceNodeId)) {
            source->recentCompleted += 1.0;
        }
        simulation.metrics_.recordProcessed(simulation.timeSeconds_ - request.creationTime);
        return;
    }

    if (request.type == RequestType::DatabaseHeavy) {
        if (request.cacheable && simulation.cacheEnabled_) {
            const bool hit = cacheHit(simulation, request.cacheKey);
            simulation.metrics_.recordCacheLookup(hit);
            if (hit) {
                request.servedFromCache = true;
                request.state = RequestState::Completed;
                request.completedTime = simulation.timeSeconds_;
                request.stateEnteredTime = simulation.timeSeconds_;
                if (Node* source = simulation.graph_.node(request.sourceNodeId)) {
                    source->recentCompleted += 1.0;
                }
                simulation.metrics_.recordProcessed(simulation.timeSeconds_ - request.creationTime);
                return;
            }
        }

        if (request.cacheable) {
            const auto replicaId = firstNodeOfType(simulation, NodeType::ReadReplica);
            if (replicaId) {
                if (Link* link = linkBetween(simulation, request.currentNodeId, *replicaId)) {
                    routeToLink(simulation, request, *link, RequestRouteStage::ToDatabase);
                    return;
                }
            }
        }

        const auto databaseId = firstNodeOfType(simulation, NodeType::Database);
        if (databaseId) {
            if (Link* link = linkBetween(simulation, request.currentNodeId, *databaseId)) {
                routeToLink(simulation, request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }

        if (Link* link = selectOutgoingLink(simulation, request.currentNodeId, RequestRouteStage::ToDatabase)) {
            routeToLink(simulation, request, *link, RequestRouteStage::ToDatabase);
            return;
        }
    }

    request.state = RequestState::Completed;
    request.completedTime = simulation.timeSeconds_;
    request.stateEnteredTime = simulation.timeSeconds_;
    if (Node* source = simulation.graph_.node(request.sourceNodeId)) {
        source->recentCompleted += 1.0;
    }
    simulation.metrics_.recordProcessed(simulation.timeSeconds_ - request.creationTime);
}

void SimulationRequestFlowSystem::routeFromDatabase(Simulation& simulation, Request& request)
{
    const auto apiId = firstNodeOfType(simulation, NodeType::ApiService);
    if (apiId) {
        if (Link* link = linkBetween(simulation, request.currentNodeId, *apiId)) {
            routeToLink(simulation, request, *link, RequestRouteStage::BackToApi);
            return;
        }
    }

    request.state = RequestState::Completed;
    request.completedTime = simulation.timeSeconds_;
    request.stateEnteredTime = simulation.timeSeconds_;
    if (Node* source = simulation.graph_.node(request.sourceNodeId)) {
        source->recentCompleted += 1.0;
    }
    simulation.metrics_.recordProcessed(simulation.timeSeconds_ - request.creationTime);
}

void SimulationRequestFlowSystem::routeToLink(Simulation& simulation, Request& request, Link& link, RequestRouteStage nextStage)
{
    request.currentLinkId = link.id;
    request.targetNodeId = link.targetNodeId;
    request.routeStage = nextStage;
    request.state = RequestState::InTransit;
    request.stateEnteredTime = simulation.timeSeconds_;
    request.transitProgress = 0.0;
    link.inFlightRequests.push_back(request.id);
}

Link* SimulationRequestFlowSystem::selectOutgoingLink(Simulation& simulation, int sourceNodeId, RequestRouteStage routeStage)
{
    std::vector<Link*> candidates;
    for (auto& link : simulation.graph_.links()) {
        if (link.enabled && link.sourceNodeId == sourceNodeId) {
            candidates.push_back(&link);
        }
    }
    if (candidates.empty()) {
        return nullptr;
    }

    const auto& evolution = simulation.scenario_.trafficProfile.evolution;
    if (!evolution.enabled || (evolution.reroutePressureSensitivity <= 0.0 && evolution.rerouteLatencySensitivity <= 0.0)) {
        return candidates.front();
    }

    Link* selected = candidates.front();
    double bestScore = std::numeric_limits<double>::max();
    for (Link* link : candidates) {
        const Node* target = simulation.graph_.node(link->targetNodeId);
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

Link* SimulationRequestFlowSystem::selectClientIngressLink(Simulation& simulation, const Node& clientNode)
{
    Link* selected = selectOutgoingLink(simulation, clientNode.id, RequestRouteStage::ToApi);
    const auto& evolution = simulation.scenario_.trafficProfile.evolution;
    if (selected == nullptr || !evolution.enabled || evolution.migrationSensitivity <= 0.0) {
        return selected;
    }

    const Node* target = simulation.graph_.node(selected->targetNodeId);
    if (target == nullptr) {
        return selected;
    }
    const double clientStress = std::clamp(1.0 - clientNode.experienceScore + target->propagatedInstability, 0.0, 1.0);
    if (clientStress <= 0.0) {
        return selected;
    }

    Link* migrated = selected;
    double bestScore = std::numeric_limits<double>::max();
    for (auto& link : simulation.graph_.links()) {
        if (!link.enabled || link.sourceNodeId != clientNode.id) {
            continue;
        }
        const Node* candidateTarget = simulation.graph_.node(link.targetNodeId);
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
    const int deterministicRoll = static_cast<int>((simulation.nextRequestId_ * 53U + static_cast<std::uint64_t>(clientNode.id) * 17U) % 100U);
    return deterministicRoll < static_cast<int>(migrationGate * 100.0) ? migrated : selected;
}

double SimulationRequestFlowSystem::trafficEvolutionMultiplierFor(const Simulation& simulation, const Node& node, double burstMultiplier)
{
    const auto& evolution = simulation.scenario_.trafficProfile.evolution;
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

double SimulationRequestFlowSystem::retryEvolutionDelayMultiplierFor(const Simulation& simulation, const Node& node)
{
    const auto& evolution = simulation.scenario_.trafficProfile.evolution;
    if (!evolution.enabled || evolution.dynamicRetrySensitivity <= 0.0) {
        return 1.0;
    }

    const double sourcePressure = std::clamp(std::max(node.retryPressure, node.timeoutPressure), 0.0, 1.0);
    return std::clamp(1.0 - evolution.dynamicRetrySensitivity * sourcePressure, 0.25, 1.0);
}

Link* SimulationRequestFlowSystem::linkBetween(Simulation& simulation, int sourceNodeId, int targetNodeId)
{
    for (auto& link : simulation.graph_.links()) {
        if (link.enabled && link.sourceNodeId == sourceNodeId && link.targetNodeId == targetNodeId) {
            return &link;
        }
    }
    return nullptr;
}

std::optional<int> SimulationRequestFlowSystem::firstNodeOfType(const Simulation& simulation, NodeType type)
{
    for (const auto& node : simulation.graph_.nodes()) {
        if (node.type == type) {
            return node.id;
        }
    }
    return std::nullopt;
}

double SimulationRequestFlowSystem::processingCost(const Simulation& simulation, const Request& request, const Node& node)
{
    if (node.type == NodeType::Database || node.type == NodeType::ReadReplica) {
        return simulation.scenario_.requestTypes.databaseCostHeavy;
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
        return simulation.scenario_.requestTypes.apiCostReturn;
    }

    return request.type == RequestType::Lightweight
        ? simulation.scenario_.requestTypes.apiCostLightweight
        : simulation.scenario_.requestTypes.apiCostDatabaseHeavy;
}

bool SimulationRequestFlowSystem::requestTimedOut(const Simulation& simulation, const Request& request)
{
    return simulation.timeSeconds_ - request.creationTime > simulation.scenario_.requestTimeoutSeconds;
}

bool SimulationRequestFlowSystem::cacheHit(Simulation& simulation, int key)
{
    expireCacheEntries(simulation);
    return std::any_of(simulation.cacheEntries_.begin(), simulation.cacheEntries_.end(), [key](const Simulation::CacheEntry& entry) {
        return entry.key == key;
    });
}

void SimulationRequestFlowSystem::storeCache(Simulation& simulation, int key)
{
    if (!simulation.cacheEnabled_) {
        return;
    }

    simulation.cacheEntries_.erase(
        std::remove_if(simulation.cacheEntries_.begin(), simulation.cacheEntries_.end(), [key](const Simulation::CacheEntry& entry) {
            return entry.key == key;
        }),
        simulation.cacheEntries_.end());

    while (static_cast<int>(simulation.cacheEntries_.size()) >= simulation.scenario_.cache.maxEntries) {
        simulation.cacheEntries_.pop_front();
    }

    simulation.cacheEntries_.push_back({key, simulation.timeSeconds_ + simulation.scenario_.cache.ttlSeconds});
}

void SimulationRequestFlowSystem::expireCacheEntries(Simulation& simulation)
{
    simulation.cacheEntries_.erase(
        std::remove_if(simulation.cacheEntries_.begin(), simulation.cacheEntries_.end(), [&simulation](const Simulation::CacheEntry& entry) {
            return entry.expiresAt <= simulation.timeSeconds_;
        }),
        simulation.cacheEntries_.end());
}


void SimulationRequestFlowSystem::pruneOldRequests(Simulation& simulation)
{
    for (auto it = simulation.requests_.begin(); it != simulation.requests_.end();) {
        const Request& request = it->second;
        const bool terminal = request.state == RequestState::Completed || request.state == RequestState::TimedOut;
        if (terminal && simulation.timeSeconds_ - request.completedTime > 3.0) {
            it = simulation.requests_.erase(it);
        } else {
            ++it;
        }
    }
}
