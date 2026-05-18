#include "simulation/Simulation.hpp"

#include "simulation/Geography.hpp"
#include "simulation/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

Simulation::Simulation(const ScenarioDefinition& scenario, SimulationConfig config)
    : config_(config)
{
    pressureAnalysis_.setConfig(config_.pressureAnalysis);
    buildFromScenario(scenario);
}

void Simulation::update(double dt)
{
    timeSystem_.update(dt);
    timeSeconds_ = timeSystem_.state().elapsedSeconds;
    runtimeSystems_.update(dt);
    expireCacheEntries();
    generateClientRequests(dt);
    updateRetryWaits();
    updateLinks(dt);
    updateProcessors(dt);
    updateNodeHealth();
    updateMetricsNodeStates();

    metrics_.setSimulationSpeed(simulationSpeed_);
    metrics_.setRuntimeSystemCounts(runtimeSystems_.enabledCount(), static_cast<int>(runtimeSystems_.states().size()));
    metrics_.setComplexity(complexityScore_, recommendedComplexityThreshold_);
    metrics_.update(dt);
    pressureAnalysis_.update(timeSeconds_, dt, graph_, metrics_.snapshot());
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
    (void)scaleApiCapacity(-1, multiplier, 3, 0.72, 1.0);
}

bool Simulation::scaleApiCapacity(int targetId, double multiplier, int maxScaleLevel, double diminishingReturn, double complexityCost)
{
    bool applied = false;
    for (auto& node : graph_.nodes()) {
        if (node.type != NodeType::ApiService) {
            continue;
        }
        if (targetId >= 0 && node.id != targetId) {
            continue;
        }
        node.maxScaleLevel = std::max(1, maxScaleLevel);
        if (node.scaleLevel >= node.maxScaleLevel) {
            continue;
        }
        const double levelEfficiency = std::pow(std::clamp(diminishingReturn, 0.1, 1.0), static_cast<double>(node.scaleLevel));
        const double effectiveMultiplier = 1.0 + (std::max(1.0, multiplier) - 1.0) * levelEfficiency;
        node.mechanicCapacityMultiplier = std::max(0.1, node.mechanicCapacityMultiplier * effectiveMultiplier);
        ++node.scaleLevel;
        applied = true;
    }
    if (applied) {
        addComplexity(complexityCost);
        refreshEffectiveCapacities();
    }
    return applied;
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

void Simulation::toggleRetries()
{
    scenario_.retries.enabled = !scenario_.retries.enabled;
}

void Simulation::resetProcessingCapacity()
{
    cacheEnabled_ = scenario_.cache.enabled;
    burstModeEnabled_ = scenario_.bursts.enabled;
    cacheEntries_.clear();
    for (auto& node : graph_.nodes()) {
        if (node.isProcessor()) {
            node.mechanicCapacityMultiplier = 1.0;
            node.eventCapacityMultiplier = 1.0;
            node.scaleLevel = 0;
        }
    }
    complexityScore_ = 0.0;
    refreshRegionSlots();
    refreshEffectiveCapacities();
}

void Simulation::setSimulationSpeed(double speed)
{
    simulationSpeed_ = std::max(0.0, speed);
    timeSystem_.setSpeed(simulationSpeed_);
    metrics_.setSimulationSpeed(simulationSpeed_);
}

void Simulation::setScenarioTrafficMultiplier(double multiplier)
{
    scenarioTrafficMultiplier_ = std::max(0.0, multiplier);
}

void Simulation::setScenarioBurst(const BurstScenario& burst)
{
    scenarioBurstOverride_ = burst;
}

void Simulation::setScenarioDatabaseCapacityMultiplier(double multiplier)
{
    scenarioDatabaseCapacityMultiplier_ = std::max(0.1, multiplier);
    for (auto& node : graph_.nodes()) {
        if (node.type == NodeType::Database) {
            node.eventCapacityMultiplier = scenarioDatabaseCapacityMultiplier_;
        }
    }
    refreshEffectiveCapacities();
}

void Simulation::setScenarioLatencyMultiplier(double multiplier)
{
    scenarioLatencyMultiplier_ = std::max(0.1, multiplier);
}

void Simulation::setScenarioDatabaseHeavyShareOverride(std::optional<double> share)
{
    scenarioDatabaseHeavyShareOverride_ = share;
}

void Simulation::setScenarioRetryDelayMultiplier(double multiplier)
{
    scenarioRetryDelayMultiplier_ = std::max(0.1, multiplier);
}

void Simulation::setLocalizedEventModifiers(std::vector<LocalizedEventModifier> modifiers)
{
    localizedEventModifiers_ = std::move(modifiers);
    refreshEffectiveCapacities();
}

void Simulation::setScenarioTime(double elapsedSeconds, double phaseElapsedSeconds, double calendarElapsedDays)
{
    timeSystem_.setScenarioElapsed(elapsedSeconds);
    timeSystem_.setPhaseElapsed(phaseElapsedSeconds);
    timeSystem_.setCalendarElapsedDays(calendarElapsedDays);
}

void Simulation::clearScenarioBurstOverride()
{
    scenarioBurstOverride_.reset();
}

void Simulation::setPaused(bool paused)
{
    timeSystem_.setPaused(paused);
}

void Simulation::setAllowedMechanics(const std::vector<MechanicType>& mechanics)
{
    allowedMechanics_.fill(false);
    if (mechanics.empty()) {
        for (const auto& definition : MechanicRegistry::definitions()) {
            const auto index = static_cast<std::size_t>(definition.type);
            if (index < allowedMechanics_.size()) {
                allowedMechanics_[index] = definition.available;
            }
        }
        return;
    }

    for (const auto mechanic : mechanics) {
        const auto index = static_cast<std::size_t>(mechanic);
        if (index < allowedMechanics_.size()) {
            allowedMechanics_[index] = true;
        }
    }
}

bool Simulation::applyTopologyMutation(const TopologyMutation& mutation)
{
    if (mutation.regionSlotUsage > 0) {
        for (const auto& node : mutation.nodesToCreate) {
            if (node.hasGeoLocation && !canUseRegionSlots(node.geoLocation.regionName, mutation.regionSlotUsage)) {
                return false;
            }
        }
    }

    std::vector<int> createdNodeIds;
    createdNodeIds.reserve(mutation.nodesToCreate.size());
    for (auto node : mutation.nodesToCreate) {
        createdNodeIds.push_back(graph_.addNode(std::move(node)));
    }
    refreshRegionSlots();

    for (const int linkId : mutation.linksToDisable) {
        if (Link* link = graph_.link(linkId)) {
            link->enabled = false;
            graph_.markTopologyChanged();
        }
    }

    for (auto link : mutation.linksToCreate) {
        for (const int createdId : createdNodeIds) {
            if (link.sourceNodeId == -1) {
                link.sourceNodeId = createdId;
            }
            if (link.targetNodeId == -1) {
                link.targetNodeId = createdId;
            }
        }
        const Node* source = graph_.node(link.sourceNodeId);
        const Node* target = graph_.node(link.targetNodeId);
        if (source != nullptr && target != nullptr && source->hasGeoLocation && target->hasGeoLocation) {
            const GeographicSystem geography;
            const double geographicLatency = geography.latencySeconds(source->geoLocation, target->geoLocation, link.baseLatencySeconds);
            link.geographicDistanceKm = MapProjection::greatCircleKilometers(source->geoLocation, target->geoLocation);
            link.geographicLatencyContributionSeconds = std::max(0.0, geographicLatency - link.baseLatencySeconds);
            link.baseLatencySeconds = geographicLatency;
        }
        graph_.addLink(std::move(link));
    }

    const bool applied = !createdNodeIds.empty() || !mutation.linksToDisable.empty() || !mutation.linksToCreate.empty();
    if (applied) {
        addComplexity(mutation.complexityCost);
    }
    return applied;
}

void Simulation::addComplexity(double amount)
{
    complexityScore_ = std::max(0.0, complexityScore_ + amount);
    metrics_.setComplexity(complexityScore_, recommendedComplexityThreshold_);
}

bool Simulation::canScaleNode(int nodeId, int maxScaleLevel) const
{
    if (nodeId >= 0) {
        const Node* node = graph_.node(nodeId);
        return node != nullptr && node->type == NodeType::ApiService && node->scaleLevel < maxScaleLevelForNode(nodeId, maxScaleLevel);
    }
    for (const auto& node : graph_.nodes()) {
        if (node.type == NodeType::ApiService && node.scaleLevel < std::max(1, maxScaleLevel)) {
            return true;
        }
    }
    return false;
}

int Simulation::scaleLevelForNode(int nodeId) const
{
    const Node* node = graph_.node(nodeId);
    return node != nullptr ? node->scaleLevel : 0;
}

int Simulation::maxScaleLevelForNode(int nodeId, int contentMaxScaleLevel) const
{
    const Node* node = graph_.node(nodeId);
    if (node == nullptr) {
        return std::max(1, contentMaxScaleLevel);
    }
    return std::max(1, contentMaxScaleLevel > 0 ? contentMaxScaleLevel : node->maxScaleLevel);
}

bool Simulation::canUseRegionSlots(const std::string& region, int slots) const
{
    if (slots <= 0 || region.empty()) {
        return true;
    }
    return regionSlotsUsed(region) + slots <= regionSlotLimit(region);
}

bool Simulation::hasAnyRegionCapacity(int slots) const
{
    if (slots <= 0 || regionSlotLimits_.empty()) {
        return true;
    }
    for (const auto& [region, limit] : regionSlotLimits_) {
        if (regionSlotsUsed(region) + slots <= limit) {
            return true;
        }
    }
    return false;
}

int Simulation::regionSlotsUsed(const std::string& region) const
{
    const auto it = regionSlotsUsed_.find(region);
    return it != regionSlotsUsed_.end() ? it->second : 0;
}

int Simulation::regionSlotLimit(const std::string& region) const
{
    const auto it = regionSlotLimits_.find(region);
    return it != regionSlotLimits_.end() ? it->second : 5;
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

const PressureSnapshot& Simulation::pressure() const
{
    return pressureAnalysis_.snapshot();
}

const PressureAnalysisSystem& Simulation::pressureAnalysis() const
{
    return pressureAnalysis_;
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

bool Simulation::retriesEnabled() const
{
    return scenario_.retries.enabled;
}

double Simulation::simulationSpeed() const
{
    return simulationSpeed_;
}

bool Simulation::isMechanicAllowed(MechanicType mechanic) const
{
    const auto index = static_cast<std::size_t>(mechanic);
    if (index >= allowedMechanics_.size()) {
        return false;
    }
    return allowedMechanics_[index];
}

const RuntimeSystems& Simulation::runtimeSystems() const
{
    return runtimeSystems_;
}

const TimeState& Simulation::timeState() const
{
    return timeSystem_.state();
}

const SimulationConfig& Simulation::config() const
{
    return config_;
}

double Simulation::complexityScore() const
{
    return complexityScore_;
}

double Simulation::recommendedComplexityThreshold() const
{
    return recommendedComplexityThreshold_;
}

void Simulation::buildFromScenario(const ScenarioDefinition& scenario)
{
    scenario_ = scenario;
    graph_ = InfrastructureGraph{};
    requests_.clear();
    cacheEntries_.clear();
    metrics_.reset();
    pressureAnalysis_.reset();
    pressureAnalysis_.setConfig(config_.pressureAnalysis);
    nextRequestId_ = 1;
    timeSeconds_ = 0.0;
    simulationSpeed_ = 1.0;
    timeSystem_.reset();
    timeSystem_.setSpeed(simulationSpeed_);
    scenarioTrafficMultiplier_ = 1.0;
    scenarioDatabaseCapacityMultiplier_ = 1.0;
    scenarioRetryDelayMultiplier_ = 1.0;
    scenarioLatencyMultiplier_ = 1.0;
    complexityScore_ = 0.0;
    cacheEnabled_ = scenario.cache.enabled;
    burstModeEnabled_ = scenario.bursts.enabled;
    scenarioBurstOverride_.reset();
    scenarioDatabaseHeavyShareOverride_.reset();
    localizedEventModifiers_.clear();
    setAllowedMechanics(scenario.allowedMechanics);
    runtimeSystems_.initialize(config_);

    for (const auto& nodeScenario : scenario.nodes) {
        const auto& definition = NodeRegistry::definition(nodeScenario.type);
        Node node;
        node.name = nodeScenario.name;
        node.type = nodeScenario.type;
        node.position = nodeScenario.position;
        if (nodeScenario.geoLocation) {
            node.geoLocation = *nodeScenario.geoLocation;
            node.hasGeoLocation = true;
            node.position = MapProjection::projectEquirectangular(node.geoLocation);
        }
        node.networkIdentity = nodeScenario.networkIdentity;
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
    refreshRegionSlots();

    for (const auto& linkScenario : scenario.links) {
        Link link;
        link.sourceNodeId = linkScenario.sourceNode;
        link.targetNodeId = linkScenario.targetNode;
        link.baseLatencySeconds = linkScenario.baseLatencySeconds;
        link.bandwidthPerSecond = linkScenario.bandwidthPerSecond;
        const Node* source = graph_.node(link.sourceNodeId);
        const Node* target = graph_.node(link.targetNodeId);
        if (source != nullptr && target != nullptr && source->hasGeoLocation && target->hasGeoLocation) {
            const GeographicSystem geography;
            const double geographicLatency = geography.latencySeconds(source->geoLocation, target->geoLocation, link.baseLatencySeconds);
            link.geographicDistanceKm = MapProjection::greatCircleKilometers(source->geoLocation, target->geoLocation);
            link.geographicLatencyContributionSeconds = std::max(0.0, geographicLatency - link.baseLatencySeconds);
            link.baseLatencySeconds = geographicLatency;
        }
        graph_.addLink(std::move(link));
    }
}

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

        Link* link = graph_.firstOutgoingLink(node.id);
        if (link == nullptr) {
            continue;
        }

        node.generationAccumulator += node.requestRatePerSecond * scenarioTrafficMultiplier_ * localizedTrafficMultiplierFor(node) * burstMultiplier * dt;
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

    if (node.type == NodeType::ReadReplica) {
        routeFromDatabase(request);
        return;
    }

    if (node.type == NodeType::Cache || node.type == NodeType::QueueBroker) {
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
        request.retryDueTime = timeSeconds_ + scenario_.retries.retryDelaySeconds * scenarioRetryDelayMultiplier_ * localizedRetryDelayMultiplier;
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

        if (Link* link = graph_.firstOutgoingLink(request.currentNodeId)) {
            routeToLink(request, *link, RequestRouteStage::ToDatabase);
            return;
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

    if (node.type == NodeType::Cache || node.type == NodeType::QueueBroker) {
        return 0.25;
    }

    if (request.routeStage == RequestRouteStage::ApiReturn) {
        return scenario_.requestTypes.apiCostReturn;
    }

    return request.type == RequestType::Lightweight
        ? scenario_.requestTypes.apiCostLightweight
        : scenario_.requestTypes.apiCostDatabaseHeavy;
}

bool Simulation::eventLocationMatches(const EventLocation& location, const Node& node) const
{
    switch (location.scope) {
    case EventLocationScope::Global:
        return true;
    case EventLocationScope::Region:
        return node.hasGeoLocation && node.geoLocation.regionName == location.region;
    case EventLocationScope::NodeType:
        return node.type == location.nodeType;
    }
    return false;
}

double Simulation::localizedTrafficMultiplierFor(const Node& node) const
{
    double multiplier = 1.0;
    for (const auto& modifier : localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.trafficMultiplier;
        }
    }
    return multiplier;
}

double Simulation::localizedCapacityMultiplierFor(const Node& node) const
{
    double multiplier = 1.0;
    for (const auto& modifier : localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.databaseCapacityMultiplier;
        }
    }
    return multiplier;
}

double Simulation::localizedRetryDelayMultiplierFor(const Node& node) const
{
    double multiplier = 1.0;
    for (const auto& modifier : localizedEventModifiers_) {
        if (eventLocationMatches(modifier.location, node)) {
            multiplier *= modifier.effect.retryDelayMultiplier;
        }
    }
    return multiplier;
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

void Simulation::refreshEffectiveCapacities()
{
    for (auto& node : graph_.nodes()) {
        if (!node.isProcessor()) {
            continue;
        }
        node.processingCapacityPerSecond = std::max(
            0.1,
            node.baseProcessingCapacityPerSecond * node.mechanicCapacityMultiplier * node.eventCapacityMultiplier * localizedCapacityMultiplierFor(node));
    }
}

void Simulation::refreshRegionSlots()
{
    regionSlotsUsed_.clear();
    regionSlotLimits_.clear();
    for (const auto& node : graph_.nodes()) {
        if (!node.hasGeoLocation || node.geoLocation.regionName.empty()) {
            continue;
        }
        regionSlotLimits_.try_emplace(node.geoLocation.regionName, 5);
        if (node.type != NodeType::ClientCluster) {
            ++regionSlotsUsed_[node.geoLocation.regionName];
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
