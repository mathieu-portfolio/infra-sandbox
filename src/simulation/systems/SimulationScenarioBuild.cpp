#include "simulation/systems/SimulationScenarioBuilder.hpp"
#include "simulation/systems/SimulationTuningSystem.hpp"
#include "simulation/systems/SimulationModifierSystem.hpp"
#include "simulation/systems/SimulationPressureSystem.hpp"
#include "simulation/systems/SimulationTopologySystem.hpp"

#include "simulation/core/Simulation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>


void SimulationScenarioBuilder::buildFromScenario(Simulation& simulation, const ScenarioDefinition& scenario)
{
    simulation.scenario_ = scenario;
    simulation.graph_ = InfrastructureGraph{};
    simulation.requests_.clear();
    simulation.cacheEntries_.clear();
    simulation.metrics_.reset();
    simulation.scenarioPressureContext_ = scenario.pressureContext;
    simulation.eventPressureContext_ = {};
    simulation.pressureState_ = simulation.scenarioPressureContext_;
    simulation.scenarioPressureSignals_ = scenario.pressureSignals;
    simulation.eventPressureSignals_.clear();
    simulation.pressureAnalysis_.reset();
    simulation.pressureAnalysis_.setConfig(simulation.config_.pressureAnalysis);
    simulation.nextRequestId_ = 1;
    simulation.timeSeconds_ = 0.0;
    simulation.simulationSpeed_ = 1.0;
    simulation.timeSystem_.reset();
    simulation.timeSystem_.setSpeed(simulation.simulationSpeed_);
    simulation.scenarioTrafficMultiplier_ = 1.0;
    simulation.scenarioDatabaseCapacityMultiplier_ = 1.0;
    simulation.scenarioRetryDelayMultiplier_ = 1.0;
    simulation.scenarioLatencyMultiplier_ = 1.0;
    simulation.complexityScore_ = 0.0;
    simulation.cacheEnabled_ = scenario.cache.enabled;
    simulation.burstModeEnabled_ = scenario.bursts.enabled;
    simulation.scenarioBurstOverride_.reset();
    simulation.scenarioDatabaseHeavyShareOverride_.reset();
    simulation.localizedEventModifiers_.clear();
    SimulationTuningSystem::setAllowedMechanics(simulation, scenario.allowedMechanics);
    simulation.runtimeSystems_.initialize(simulation.config_);

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
        simulation.graph_.addNode(std::move(node));
    }
    SimulationTopologySystem::refreshRegionSlots(simulation);

    for (const auto& linkScenario : scenario.links) {
        Link link;
        link.sourceNodeId = linkScenario.sourceNode;
        link.targetNodeId = linkScenario.targetNode;
        link.baseLatencySeconds = linkScenario.baseLatencySeconds;
        link.bandwidthPerSecond = linkScenario.bandwidthPerSecond;
        const Node* source = simulation.graph_.node(link.sourceNodeId);
        const Node* target = simulation.graph_.node(link.targetNodeId);
        if (source != nullptr && target != nullptr && source->hasGeoLocation && target->hasGeoLocation) {
            const GeographicSystem geography;
            const double geographicLatency = geography.latencySeconds(source->geoLocation, target->geoLocation, link.baseLatencySeconds);
            link.geographicDistanceKm = MapProjection::greatCircleKilometers(source->geoLocation, target->geoLocation);
            link.geographicLatencyContributionSeconds = std::max(0.0, geographicLatency - link.baseLatencySeconds);
            link.baseLatencySeconds = geographicLatency;
        }
        simulation.graph_.addLink(std::move(link));
    }
}
