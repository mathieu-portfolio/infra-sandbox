#include "simulation/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

Simulation::Simulation(const ScenarioDefinition& scenario, SimulationConfig config)
    : config_(config)
{
    buildFromScenario(scenario);
}

void Simulation::update(double dt)
{
    timeSeconds_ += dt;
    layerSystems_.update(dt);
    expireCacheEntries();
    generateClientRequests(dt);
    updateRetryWaits();
    updateLinks(dt);
    updateProcessors(dt);
    updateNodeHealth();
    updateMetricsNodeStates();

    metrics_.setSimulationSpeed(simulationSpeed_);
    metrics_.setLayerSystemCounts(layerSystems_.enabledCount(), static_cast<int>(layerSystems_.states().size()));
    metrics_.update(dt);
    pruneOldRequests();
}

void Simulation::adjustClientRequestRates(double deltaPerSecond)
{
    for (auto& node : graph_.nodes()) {
        if (NodeRegistry::generatesRequests(node.type)) {
            node.requestRatePerSecond = std::max(0.0, node.requestRatePerSecond + deltaPerSecond);
            node.baseRequestRatePerSecond = node.requestRatePerSecond;
        }
    }
}

void Simulation::scaleApiCapacity(double multiplier)
{
    for (auto& node : graph_.nodes()) {
        if (node.type == NodeType::ApiService) {
            node.processingCapacityPerSecond = std::max(0.1, node.processingCapacityPerSecond * multiplier);
        }
    }
}

void Simulation::toggleCache()
{
    cacheEnabled_ = !cacheEnabled_;
}

void Simulation::clearCache()
{
    cacheEntries_.clear();
}

void Simulation::toggleBurstMode()
{
    burstModeEnabled_ = !burstModeEnabled_;
}

void Simulation::resetProcessingCapacity()
{
    cacheEnabled_ = scenario_.cache.enabled;
    burstModeEnabled_ = scenario_.bursts.enabled;
    cacheEntries_.clear();
    for (auto& node : graph_.nodes()) {
        if (node.isProcessor()) {
            node.processingCapacityPerSecond = node.baseProcessingCapacityPerSecond;
        }
    }
}

void Simulation::setSimulationSpeed(double speed)
{
    simulationSpeed_ = std::max(0.0, speed);
    metrics_.setSimulationSpeed(simulationSpeed_);
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

bool Simulation::cacheEnabled() const
{
    return cacheEnabled_;
}

bool Simulation::burstModeEnabled() const
{
    return burstModeEnabled_;
}

double Simulation::simulationSpeed() const
{
    return simulationSpeed_;
}

const LayerSystems& Simulation::layerSystems() const
{
    return layerSystems_;
}

const SimulationConfig& Simulation::config() const
{
    return config_;
}

void Simulation::buildFromScenario(const ScenarioDefinition& scenario)
{
    scenario_ = scenario;
    graph_ = InfrastructureGraph{};
    requests_.clear();
    cacheEntries_.clear();
    metrics_.reset();
    nextRequestId_ = 1;
    timeSeconds_ = 0.0;
    simulationSpeed_ = 1.0;
    cacheEnabled_ = scenario.cache.enabled;
    burstModeEnabled_ = scenario.bursts.enabled;
    layerSystems_.initialize(config_);

    for (const auto& nodeScenario : scenario.nodes) {
        const auto& definition = NodeRegistry::definition(nodeScenario.type);
        Node node;
        node.name = nodeScenario.name;
        node.type = nodeScenario.type;
        node.position = nodeScenario.position;
        node.requestRatePerSecond = nodeScenario.requestRatePerSecond > 0.0
            ? nodeScenario.requestRatePerSecond
            : definition.defaultRequestRatePerSecond;
        node.baseRequestRatePerSecond = node.requestRatePerSecond;
        node.processingCapacityPerSecond = nodeScenario.processingCapacityPerSecond > 0.0
            ? nodeScenario.processingCapacityPerSecond
            : definition.defaultProcessingCapacityPerSecond;
        node.baseProcessingCapacityPerSecond = node.processingCapacityPerSecond;
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
    double burstMultiplier = 1.0;
    if (burstModeEnabled_) {
        const double phase = std::fmod(timeSeconds_, scenario_.bursts.periodSeconds);
        if (phase < scenario_.bursts.durationSeconds) {
            burstMultiplier = scenario_.bursts.multiplier;
        }
    }

    for (auto& node : graph_.nodes()) {
        if (!NodeRegistry::generatesRequests(node.type)) {
            continue;
        }

        Link* link = graph_.firstOutgoingLink(node.id);
        if (link == nullptr) {
            continue;
        }

        node.generationAccumulator += node.requestRatePerSecond * burstMultiplier * dt;
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
    request.routeStage = RequestRouteStage::ToApi;
    request.creationTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;

    const int sequence = static_cast<int>(request.id % 100);
    const int lightThreshold = static_cast<int>(scenario_.requestTypes.lightweightShare * 100.0);
    request.type = sequence < lightThreshold ? RequestType::Lightweight : RequestType::DatabaseHeavy;
    const int cacheRoll = static_cast<int>((request.id * 37U) % 100U);
    request.cacheKey = static_cast<int>(request.id % std::max(1, scenario_.requestTypes.cacheKeySpace));
    request.cacheable = request.type == RequestType::DatabaseHeavy
        && cacheRoll < static_cast<int>(scenario_.requestTypes.databaseHeavyCacheableShare * 100.0);

    link.inFlightRequests.push_back(request.id);
    requests_.emplace(request.id, request);
    metrics_.recordGenerated();
}

void Simulation::retryRequest(Request& request)
{
    Link* link = graph_.firstOutgoingLink(request.sourceNodeId);
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

            const double latency = std::max(0.01, link.baseLatencySeconds);
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
        node.processingAccumulator += node.processingCapacityPerSecond * dt;

        while (!node.queue.empty()) {
            const auto requestId = node.queue.front();
            auto requestIt = requests_.find(requestId);
            if (requestIt == requests_.end()) {
                node.queue.pop_front();
                continue;
            }

            Request& request = requestIt->second;
            const double cost = processingCost(request, node);
            if (node.processingAccumulator < cost) {
                break;
            }

            node.queue.pop_front();
            node.processingAccumulator -= cost;

            if (requestTimedOut(request)) {
                timeOutRequest(request);
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

    request.state = RequestState::Completed;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
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
        request.retryDueTime = timeSeconds_ + scenario_.retries.retryDelaySeconds;
        request.stateEnteredTime = timeSeconds_;
        metrics_.recordTimedOut(timeSeconds_ - request.creationTime);
        return;
    }

    request.state = RequestState::TimedOut;
    request.currentLinkId = -1;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
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
                metrics_.recordProcessed(timeSeconds_ - request.creationTime);
                return;
            }
        }

        const auto databaseId = firstNodeOfType(NodeType::Database);
        if (databaseId) {
            if (Link* link = linkBetween(request.currentNodeId, *databaseId)) {
                routeToLink(request, *link, RequestRouteStage::ToDatabase);
                return;
            }
        }
    }

    request.state = RequestState::Completed;
    request.completedTime = timeSeconds_;
    request.stateEnteredTime = timeSeconds_;
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

Link* Simulation::linkBetween(int sourceNodeId, int targetNodeId)
{
    for (auto& link : graph_.links()) {
        if (link.sourceNodeId == sourceNodeId && link.targetNodeId == targetNodeId) {
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
    if (node.type == NodeType::Database) {
        return scenario_.requestTypes.databaseCostHeavy;
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

void Simulation::updateNodeHealth()
{
    for (auto& node : graph_.nodes()) {
        if (!node.isProcessor()) {
            node.health = HealthState::Healthy;
            continue;
        }

        if (node.queue.size() > static_cast<std::size_t>(node.processingCapacityPerSecond * scenario_.requestTimeoutSeconds * 0.75)) {
            node.health = HealthState::Failing;
        } else if (node.currentUtilization > 0.85 || node.queue.size() > static_cast<std::size_t>(node.processingCapacityPerSecond)) {
            node.health = HealthState::Saturated;
        } else {
            node.health = HealthState::Healthy;
        }
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
