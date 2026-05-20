#include "simulation/topology/TopologyMutation.hpp"

#include "core/topology/Geography.hpp"
#include "simulation/core/Simulation.hpp"

#include <algorithm>

namespace {
Node makeNode(NodeType type, std::string name, const PlacementOption& option)
{
    const auto& definition = NodeRegistry::definition(type);
    Node node;
    node.name = std::move(name);
    node.type = type;
    node.geoLocation = option.location;
    node.hasGeoLocation = true;
    node.position = MapProjection::projectEquirectangular(option.location);
    node.networkIdentity.hostname = option.id + ".infrastructure.local";
    node.networkIdentity.ipAddress = "10.30.0." + std::to_string(static_cast<int>(option.id.size()) + 10);
    node.networkIdentity.endpoint = "https://" + option.id + ".example.net";
    node.processingCapacityPerSecond = definition.defaultProcessingCapacityPerSecond;
    node.baseProcessingCapacityPerSecond = node.processingCapacityPerSecond;
    return node;
}

const Node* firstNodeOfType(const Simulation& simulation, NodeType type)
{
    for (const auto& node : simulation.graph().nodes()) {
        if (node.type == type) {
            return &node;
        }
    }
    return nullptr;
}

const Link* firstEnabledLinkBetween(const Simulation& simulation, int source, int target)
{
    for (const auto& link : simulation.graph().links()) {
        if (link.enabled && link.sourceNodeId == source && link.targetNodeId == target) {
            return &link;
        }
    }
    return nullptr;
}

Link makeLink(int source, int target, double baseLatencySeconds = 0.22)
{
    Link link;
    link.sourceNodeId = source;
    link.targetNodeId = target;
    link.baseLatencySeconds = baseLatencySeconds;
    link.bandwidthPerSecond = 100.0;
    return link;
}

void applyGeoLatency(Link& link, const Simulation& simulation, const Node& source, const Node& target)
{
    if (!source.hasGeoLocation || !target.hasGeoLocation) {
        return;
    }
    const GeographicSystem geography;
    const double geographicLatency = geography.latencySeconds(source.geoLocation, target.geoLocation, link.baseLatencySeconds);
    link.geographicDistanceKm = MapProjection::greatCircleKilometers(source.geoLocation, target.geoLocation);
    link.geographicLatencyContributionSeconds = std::max(0.0, geographicLatency - link.baseLatencySeconds);
    link.baseLatencySeconds = geographicLatency;
    (void)simulation;
}
}

std::vector<PlacementOption> PlacementCandidateGenerator::generate(const Simulation&, TopologyMutationType type) const
{
    std::vector<PlacementOption> options{
        {
            .id = "north-america",
            .displayName = "North America",
            .location = {39.5, -98.35, "NorthAmerica"},
            .latencyImpact = "Best for North American users and regional failover",
            .trafficImpact = "Localizes NA traffic and reduces transatlantic load",
            .resourceCost = "Medium",
            .complexityImpact = "Medium",
        },
        {
            .id = "europe",
            .displayName = "Europe",
            .location = {50.1, 8.7, "Europe"},
            .latencyImpact = "Best for European API and DB paths",
            .trafficImpact = "Keeps traffic near the central backend",
            .resourceCost = "Low",
            .complexityImpact = "Low",
        },
        {
            .id = "asia-pacific",
            .displayName = "Asia Pacific",
            .location = {1.35, 103.8, "AsiaPacific"},
            .latencyImpact = "Prepares APAC expansion and distant-user traffic",
            .trafficImpact = "Localizes APAC demand and reduces global round trips",
            .resourceCost = "Medium",
            .complexityImpact = "Medium",
        },
        {
            .id = "south-america",
            .displayName = "South America",
            .location = {-23.55, -46.63, "SouthAmerica"},
            .latencyImpact = "Improves latency for South American users",
            .trafficImpact = "Creates a regional absorption point for southern traffic",
            .resourceCost = "Medium",
            .complexityImpact = "Medium",
        },
        {
            .id = "africa",
            .displayName = "Africa",
            .location = {-1.29, 36.82, "Africa"},
            .latencyImpact = "Improves coverage for African and nearby users",
            .trafficImpact = "Adds a regional buffer for underserved routes",
            .resourceCost = "Medium",
            .complexityImpact = "Medium",
        },
        {
            .id = "oceania",
            .displayName = "Oceania",
            .location = {-33.86, 151.21, "Oceania"},
            .latencyImpact = "Improves latency for far-edge Oceania users",
            .trafficImpact = "Reduces extreme long-haul traffic paths",
            .resourceCost = "High",
            .complexityImpact = "Medium",
        },
    };

    (void)type;
    return options;
}

MutationPreview MutationValidator::preview(const Simulation& simulation, TopologyMutationType type, const PlacementOption& option) const
{
    MutationPreview preview;
    preview.mutation.type = type;
    preview.mutation.placement = option;
    preview.mutation.description = topologyMutationName(type);

    const Node* api = firstNodeOfType(simulation, NodeType::ApiService);
    const Node* database = firstNodeOfType(simulation, NodeType::Database);
    if (api == nullptr) {
        preview.validationMessage = "Requires an API service.";
        return preview;
    }

    if ((type == TopologyMutationType::AddReadReplica || type == TopologyMutationType::AddQueue || type == TopologyMutationType::AddCache) && database == nullptr) {
        preview.validationMessage = "Requires a persistence node.";
        return preview;
    }

    if ((type == TopologyMutationType::AddRegionalCache || type == TopologyMutationType::AddCache) && option.location.regionName.empty()) {
        preview.validationMessage = "Cache placement requires a region.";
        return preview;
    }

    Node created;
    switch (type) {
    case TopologyMutationType::AddCache:
        created = makeNode(NodeType::Cache, option.displayName + " cache", option);
        if (database != nullptr) {
            if (const Link* apiToDb = firstEnabledLinkBetween(simulation, api->id, database->id)) {
                preview.mutation.linksToDisable.push_back(apiToDb->id);
            }
            auto apiToCache = makeLink(api->id, -1, 0.12);
            auto cacheToDb = makeLink(-1, database->id, 0.18);
            applyGeoLatency(apiToCache, simulation, *api, created);
            applyGeoLatency(cacheToDb, simulation, created, *database);
            preview.mutation.linksToCreate.push_back(apiToCache);
            preview.mutation.linksToCreate.push_back(cacheToDb);
        }
        break;
    case TopologyMutationType::AddReadReplica:
        created = makeNode(NodeType::ReadReplica, option.displayName + " replica", option);
        created.processingCapacityPerSecond = database != nullptr ? std::max(1.0, database->processingCapacityPerSecond * 0.65) : 2.0;
        created.baseProcessingCapacityPerSecond = created.processingCapacityPerSecond;
        if (database != nullptr) {
            auto apiToReplica = makeLink(api->id, -1, 0.22);
            auto replicaToApi = makeLink(-1, api->id, 0.22);
            applyGeoLatency(apiToReplica, simulation, *api, created);
            applyGeoLatency(replicaToApi, simulation, created, *api);
            preview.mutation.linksToCreate.push_back(apiToReplica);
            preview.mutation.linksToCreate.push_back(replicaToApi);
        }
        break;
    case TopologyMutationType::AddQueue:
        created = makeNode(NodeType::QueueBroker, option.displayName + " queue", option);
        if (database != nullptr) {
            if (const Link* apiToDb = firstEnabledLinkBetween(simulation, api->id, database->id)) {
                preview.mutation.linksToDisable.push_back(apiToDb->id);
            }
            preview.mutation.linksToCreate.push_back(makeLink(api->id, -1, 0.1));
            preview.mutation.linksToCreate.push_back(makeLink(-1, database->id, 0.2));
        }
        break;
    case TopologyMutationType::AddRegionalCache:
        created = makeNode(NodeType::CDNEdge, option.displayName + " edge cache", option);
        preview.mutation.linksToCreate.push_back(makeLink(-1, api->id, 0.28));
        break;
    }

    preview.mutation.nodesToCreate.push_back(created);
    preview.valid = true;
    preview.validationMessage = "Valid placement.";
    return preview;
}

bool TopologyBuilder::apply(Simulation& simulation, const TopologyMutation& mutation) const
{
    return simulation.applyTopologyMutation(mutation);
}

const char* topologyMutationName(TopologyMutationType type)
{
    switch (type) {
    case TopologyMutationType::AddCache:
        return "Add Cache";
    case TopologyMutationType::AddReadReplica:
        return "Add Replica";
    case TopologyMutationType::AddQueue:
        return "Add Queue";
    case TopologyMutationType::AddRegionalCache:
        return "Add Regional Cache";
    }
    return "Unknown Mutation";
}
