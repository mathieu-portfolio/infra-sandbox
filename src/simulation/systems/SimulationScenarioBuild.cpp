#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


void Simulation::buildFromScenario(const ScenarioDefinition& scenario)
{
    scenario_ = scenario;
    graph_ = InfrastructureGraph{};
    requests_.clear();
    cacheEntries_.clear();
    metrics_.reset();
    pressureState_ = {};
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
        node.computeWeight = nodeScenario.resourceProfile.compute;
        node.memoryWeight = nodeScenario.resourceProfile.memory;
        node.storageWeight = nodeScenario.resourceProfile.storage;
        node.networkWeight = nodeScenario.resourceProfile.network;
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
