#include "content/ContentRegistry.hpp"

#include "content/Json.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_map>

namespace content {
namespace {
std::string readFile(const std::filesystem::path& path, std::string& error)
{
    std::ifstream input(path);
    if (!input) {
        error = "Unable to open " + path.string();
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

Json loadJsonFile(const std::filesystem::path& path, ContentLoadResult& result)
{
    std::string error;
    const std::string text = readFile(path, error);
    if (!error.empty()) {
        result.errors.push_back(error);
        return Json{};
    }
    auto parsed = parseJson(text);
    if (!parsed.error.empty()) {
        result.errors.push_back(path.string() + ": " + parsed.error);
        return Json{};
    }
    return parsed.value;
}

std::string stringAt(const Json& object, const std::string& key, const std::string& fallback = {})
{
    if (const Json* value = object.find(key); value != nullptr && value->isString()) {
        return value->asString();
    }
    return fallback;
}

double numberAt(const Json& object, const std::string& key, double fallback = 0.0)
{
    if (const Json* value = object.find(key); value != nullptr && value->isNumber()) {
        return value->asNumber();
    }
    return fallback;
}

bool boolAt(const Json& object, const std::string& key, bool fallback = false)
{
    if (const Json* value = object.find(key); value != nullptr && value->isBool()) {
        return value->asBool();
    }
    return fallback;
}

std::vector<std::string> stringsAt(const Json& object, const std::string& key)
{
    std::vector<std::string> values;
    const Json* array = object.find(key);
    if (array == nullptr || !array->isArray()) {
        return values;
    }
    for (const auto& entry : array->asArray()) {
        if (entry.isString()) {
            values.push_back(entry.asString());
        }
    }
    return values;
}

ProgressionTier progressionTierFromId(const std::string& id)
{
    if (id == "foundations") return ProgressionTier::Foundations;
    if (id == "local_scale") return ProgressionTier::LocalScale;
    if (id == "state_and_cache") return ProgressionTier::StateAndCache;
    if (id == "failure_feedback") return ProgressionTier::FailureFeedback;
    if (id == "geographic_scale") return ProgressionTier::GeographicScale;
    if (id == "distributed_systems") return ProgressionTier::DistributedSystems;
    if (id == "complexity") return ProgressionTier::Complexity;
    return ProgressionTier::Foundations;
}

ScenarioArchetype archetypeFromId(const std::string& id)
{
    if (id == "first_request") return ScenarioArchetype::FirstRequest;
    if (id == "local_startup") return ScenarioArchetype::LocalStartup;
    if (id == "database_bottleneck" || id == "database_pressure") return ScenarioArchetype::DatabasePressure;
    if (id == "burst_traffic") return ScenarioArchetype::BurstTraffic;
    if (id == "transatlantic_latency") return ScenarioArchetype::TransatlanticLatency;
    if (id == "global_read_platform") return ScenarioArchetype::GlobalReadPlatform;
    return ScenarioArchetype::LocalStartup;
}

EducationalFocus focusFromId(const std::string& id)
{
    if (id == "queues") return EducationalFocus::Queues;
    if (id == "caching") return EducationalFocus::Caching;
    if (id == "scaling") return EducationalFocus::Scaling;
    if (id == "reliability") return EducationalFocus::Reliability;
    if (id == "persistence") return EducationalFocus::Persistence;
    if (id == "latency") return EducationalFocus::Latency;
    if (id == "geography") return EducationalFocus::Geography;
    return EducationalFocus::Scaling;
}

PressureCategory pressureFromId(const std::string& id)
{
    if (id == "traffic") return PressureCategory::TrafficPressure;
    if (id == "queue") return PressureCategory::QueuePressure;
    if (id == "compute") return PressureCategory::ComputePressure;
    if (id == "persistence") return PressureCategory::PersistencePressure;
    if (id == "retry") return PressureCategory::RetryPressure;
    if (id == "latency") return PressureCategory::LatencyPressure;
    if (id == "failure") return PressureCategory::FailurePressure;
    return PressureCategory::None;
}

MechanicType mechanicFromId(const std::string& id)
{
    if (id == "scale_up") return MechanicType::ScaleUp;
    if (id == "scale_out") return MechanicType::ScaleOut;
    if (id == "enable_cache") return MechanicType::EnableCache;
    if (id == "clear_cache") return MechanicType::ClearCache;
    if (id == "toggle_retries") return MechanicType::ToggleRetries;
    if (id == "add_cache") return MechanicType::AddCache;
    if (id == "add_queue") return MechanicType::AddQueue;
    if (id == "add_load_balancer") return MechanicType::AddLoadBalancer;
    if (id == "add_read_replica") return MechanicType::AddReadReplica;
    if (id == "add_regional_cache") return MechanicType::AddRegionalCache;
    if (id == "throttle_traffic") return MechanicType::ThrottleTraffic;
    if (id == "enable_tracing") return MechanicType::EnableTracing;
    return MechanicType::ScaleUp;
}

bool knownMechanicId(const std::string& id)
{
    static const std::set<std::string> ids{
        "scale_up", "scale_out", "enable_cache", "clear_cache", "toggle_retries", "add_cache",
        "add_queue", "add_load_balancer", "add_read_replica", "add_regional_cache",
        "throttle_traffic", "enable_tracing",
    };
    return ids.contains(id);
}

TopologyMutationType mutationFromId(const std::string& id)
{
    if (id == "add_read_replica") return TopologyMutationType::AddReadReplica;
    if (id == "add_queue") return TopologyMutationType::AddQueue;
    if (id == "add_regional_cache") return TopologyMutationType::AddRegionalCache;
    return TopologyMutationType::AddCache;
}

bool knownMutationId(const std::string& id)
{
    static const std::set<std::string> ids{"add_cache", "add_read_replica", "add_queue", "add_regional_cache"};
    return ids.contains(id);
}

NodeType nodeTypeFromId(const std::string& id)
{
    if (id == "client_cluster") return NodeType::ClientCluster;
    if (id == "api_service") return NodeType::ApiService;
    if (id == "database") return NodeType::Database;
    if (id == "cache") return NodeType::Cache;
    if (id == "read_replica") return NodeType::ReadReplica;
    if (id == "queue") return NodeType::QueueBroker;
    if (id == "cdn_edge") return NodeType::CDNEdge;
    if (id == "load_balancer") return NodeType::LoadBalancer;
    return NodeType::ApiService;
}

bool knownNodeTypeId(const std::string& id)
{
    static const std::set<std::string> ids{
        "client_cluster", "api_service", "database", "cache", "read_replica", "queue", "cdn_edge", "load_balancer",
    };
    return ids.contains(id);
}

TrafficProfileType trafficTypeFromId(const std::string& id)
{
    if (id == "gradual_growth") return TrafficProfileType::GradualGrowth;
    if (id == "bursty") return TrafficProfileType::Bursty;
    if (id == "periodic_spike") return TrafficProfileType::PeriodicSpikes;
    if (id == "regional_growth") return TrafficProfileType::GradualGrowth;
    return TrafficProfileType::Constant;
}

ScenarioObjectiveType objectiveTypeFromId(const std::string& id)
{
    if (id == "metric_threshold" || id == "max_latency") return ScenarioObjectiveType::MaxLatency;
    if (id == "max_error_rate") return ScenarioObjectiveType::MaxErrorRate;
    if (id == "min_throughput") return ScenarioObjectiveType::MinThroughput;
    if (id == "stabilize_metric" || id == "stabilize_queues") return ScenarioObjectiveType::StabilizeQueues;
    if (id == "recover_after_event") return ScenarioObjectiveType::StabilizeQueues;
    return ScenarioObjectiveType::SurviveDuration;
}

EventCategory eventCategoryFromId(const std::string& id)
{
    if (id == "traffic") return EventCategory::TrafficEvent;
    if (id == "infrastructure") return EventCategory::InfrastructureEvent;
    if (id == "reliability") return EventCategory::ReliabilityEvent;
    if (id == "geographic") return EventCategory::GeographicEvent;
    if (id == "demand") return EventCategory::DemandEvent;
    if (id == "failure") return EventCategory::FailureEvent;
    if (id == "recovery") return EventCategory::RecoveryEvent;
    if (id == "educational") return EventCategory::EducationalEvent;
    return EventCategory::TrafficEvent;
}

EventTriggerType triggerTypeFromId(const std::string& id)
{
    if (id == "metric_threshold") return EventTriggerType::MetricThreshold;
    if (id == "pressure_threshold") return EventTriggerType::PressureThreshold;
    if (id == "scenario_phase") return EventTriggerType::ScenarioPhase;
    return EventTriggerType::TimeBased;
}

EventMetric metricFromId(const std::string& id)
{
    if (id == "timeout_rate") return EventMetric::TimeoutRate;
    if (id == "retry_rate") return EventMetric::RetryRate;
    if (id == "database_queue") return EventMetric::DatabaseQueue;
    if (id == "api_queue") return EventMetric::ApiQueue;
    if (id == "cache_hit_rate") return EventMetric::CacheHitRate;
    return EventMetric::AverageLatency;
}

bool knownMetricId(const std::string& id)
{
    static const std::set<std::string> ids{
        "average_latency", "timeout_rate", "retry_rate", "database_queue", "api_queue", "cache_hit_rate",
    };
    return ids.contains(id);
}

EventEffectType effectTypeFromId(const std::string& id)
{
    if (id == "modify_traffic_rate") return EventEffectType::TrafficSpike;
    if (id == "modify_burst_intensity") return EventEffectType::TrafficSpike;
    if (id == "change_request_mix") return EventEffectType::ViralGrowth;
    if (id == "degrade_node_capacity") return EventEffectType::DatabaseSlowdown;
    if (id == "unlock_intervention") return EventEffectType::MechanicUnlock;
    if (id == "emit_feedback") return EventEffectType::PartialRecovery;
    return EventEffectType::TrafficSpike;
}

ScenarioModifierType modifierTypeFromId(const std::string& id)
{
    if (id == "read_heavy_behavior") return ScenarioModifierType::ReadHeavyBehavior;
    if (id == "aggressive_retries") return ScenarioModifierType::AggressiveRetries;
    if (id == "regional_traffic_spike") return ScenarioModifierType::RegionalTrafficSpike;
    if (id == "slow_database_window") return ScenarioModifierType::SlowDatabaseWindow;
    return ScenarioModifierType::MobileRefreshWave;
}

template <typename T, typename F>
std::vector<T> mappedStrings(const Json& object, const std::string& key, F mapper)
{
    std::vector<T> values;
    for (const auto& id : stringsAt(object, key)) {
        values.push_back(mapper(id));
    }
    return values;
}

NetworkIdentity parseIdentity(const Json& object)
{
    return {
        stringAt(object, "hostname"),
        stringAt(object, "ip_address"),
        stringAt(object, "endpoint"),
    };
}

GeoLocation parseGeo(const Json& object)
{
    return {
        numberAt(object, "latitude"),
        numberAt(object, "longitude"),
        stringAt(object, "region"),
    };
}

BurstScenario parseBurst(const Json& object)
{
    return {
        boolAt(object, "enabled"),
        numberAt(object, "multiplier", 1.0),
        numberAt(object, "period_seconds", 12.0),
        numberAt(object, "duration_seconds", 3.0),
    };
}

EventDefinition parseEvent(const Json& object)
{
    EventDefinition event;
    event.id = stringAt(object, "id");
    event.displayName = stringAt(object, "display_name", event.id);
    event.name = event.displayName;
    event.description = stringAt(object, "description");
    event.tags = stringsAt(object, "tags");
    event.category = eventCategoryFromId(stringAt(object, "category"));
    if (const Json* trigger = object.find("trigger")) {
        event.trigger.type = triggerTypeFromId(stringAt(*trigger, "type"));
        event.trigger.timeSeconds = numberAt(*trigger, "time_seconds");
        event.trigger.metric = metricFromId(stringAt(*trigger, "metric"));
        event.trigger.pressure = pressureFromId(stringAt(*trigger, "pressure"));
        event.trigger.threshold = numberAt(*trigger, "threshold");
        event.trigger.phaseIndex = static_cast<int>(numberAt(*trigger, "phase_index", -1.0));
        event.trigger.delaySeconds = numberAt(*trigger, "delay_seconds");
    }
    if (const Json* effect = object.find("effect")) {
        event.effect.type = effectTypeFromId(stringAt(*effect, "type"));
        event.effect.trafficMultiplier = numberAt(*effect, "traffic_multiplier", 1.0);
        event.effect.burstMultiplier = numberAt(*effect, "burst_multiplier", 1.0);
        event.effect.databaseCapacityMultiplier = numberAt(*effect, "database_capacity_multiplier", 1.0);
        event.effect.retryDelayMultiplier = numberAt(*effect, "retry_delay_multiplier", 1.0);
        if (const Json* share = effect->find("database_heavy_share"); share != nullptr && share->isNumber()) {
            event.effect.databaseHeavyShare = share->asNumber();
        }
        event.effect.unlockMechanics = mappedStrings<MechanicType>(*effect, "unlock_interventions", mechanicFromId);
    }
    event.durationSeconds = numberAt(object, "duration_seconds", 10.0);
    event.repeatable = boolAt(object, "repeatable");
    return event;
}

ScenarioObjective parseObjective(const Json& object)
{
    ScenarioObjective objective;
    objective.id = stringAt(object, "id");
    objective.displayName = stringAt(object, "display_name", objective.id);
    objective.description = stringAt(object, "description");
    objective.tags = stringsAt(object, "tags");
    objective.type = objectiveTypeFromId(stringAt(object, "type"));
    objective.summary = stringAt(object, "summary", objective.displayName);
    objective.threshold = numberAt(object, "threshold");
    objective.durationSeconds = numberAt(object, "duration_seconds");
    return objective;
}

TrafficProfile parseTraffic(const Json& object)
{
    TrafficProfile profile;
    profile.id = stringAt(object, "id");
    profile.displayName = stringAt(object, "display_name", profile.id);
    profile.description = stringAt(object, "description");
    profile.tags = stringsAt(object, "tags");
    profile.name = profile.displayName;
    profile.type = trafficTypeFromId(stringAt(object, "type"));
    profile.baseMultiplier = numberAt(object, "base_multiplier", 1.0);
    profile.growthPerSecond = numberAt(object, "growth_per_second");
    return profile;
}

ScenarioModifierDefinition parseModifier(const Json& object, const std::unordered_map<std::string, EventDefinition>& events)
{
    ScenarioModifierDefinition modifier;
    modifier.id = stringAt(object, "id");
    modifier.displayName = stringAt(object, "display_name", modifier.id);
    modifier.name = modifier.displayName;
    modifier.description = stringAt(object, "description");
    modifier.tags = stringsAt(object, "tags");
    modifier.type = modifierTypeFromId(modifier.id);
    modifier.selectionWeight = numberAt(object, "selection_weight", 1.0);
    modifier.trafficMultiplier = numberAt(object, "traffic_multiplier", 1.0);
    if (const Json* share = object.find("database_heavy_share"); share != nullptr && share->isNumber()) {
        modifier.databaseHeavyShare = share->asNumber();
    }
    if (const Json* burst = object.find("burst_override"); burst != nullptr && burst->isObject()) {
        modifier.burstOverride = parseBurst(*burst);
    }
    for (const auto& eventId : stringsAt(object, "events")) {
        if (const auto it = events.find(eventId); it != events.end()) {
            modifier.events.push_back(it->second);
        }
    }
    return modifier;
}

std::vector<NodeScenario> parseNodes(const Json& topology)
{
    std::vector<NodeScenario> nodes;
    if (const Json* array = topology.find("nodes"); array != nullptr && array->isArray()) {
        for (const auto& entry : array->asArray()) {
            NodeScenario node;
            node.id = stringAt(entry, "id");
            node.name = stringAt(entry, "display_name", node.id);
            node.type = nodeTypeFromId(stringAt(entry, "type"));
            if (const Json* geo = entry.find("geo"); geo != nullptr && geo->isObject()) {
                node.geoLocation = parseGeo(*geo);
            }
            if (const Json* identity = entry.find("network_identity"); identity != nullptr && identity->isObject()) {
                node.networkIdentity = parseIdentity(*identity);
            }
            node.requestRatePerSecond = numberAt(entry, "request_rate_per_second");
            node.processingCapacityPerSecond = numberAt(entry, "processing_capacity_per_second");
            node.timeoutSeconds = numberAt(entry, "timeout_seconds", 6.0);
            nodes.push_back(std::move(node));
        }
    }
    return nodes;
}

std::vector<LinkScenario> parseLinks(const Json& topology, const std::vector<NodeScenario>& nodes)
{
    std::unordered_map<std::string, int> nodeIndexes;
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        nodeIndexes[nodes[static_cast<std::size_t>(i)].id] = i;
    }
    std::vector<LinkScenario> links;
    if (const Json* array = topology.find("links"); array != nullptr && array->isArray()) {
        for (const auto& entry : array->asArray()) {
            LinkScenario link;
            const auto source = nodeIndexes.find(stringAt(entry, "source"));
            const auto target = nodeIndexes.find(stringAt(entry, "target"));
            link.sourceNode = source != nodeIndexes.end() ? source->second : 0;
            link.targetNode = target != nodeIndexes.end() ? target->second : 0;
            link.baseLatencySeconds = numberAt(entry, "base_latency_seconds", 0.6);
            link.bandwidthPerSecond = numberAt(entry, "bandwidth_per_second", 100.0);
            links.push_back(link);
        }
    }
    return links;
}

void applyScenarioOverrides(ScenarioDefinition& scenario, const Json& object)
{
    if (const Json* nodes = object.find("node_overrides"); nodes != nullptr && nodes->isArray()) {
        for (const auto& overrideNode : nodes->asArray()) {
            const std::string id = stringAt(overrideNode, "id");
            auto it = std::find_if(scenario.nodes.begin(), scenario.nodes.end(), [&id](const NodeScenario& node) {
                return node.id == id;
            });
            if (it == scenario.nodes.end()) {
                continue;
            }
            if (overrideNode.find("request_rate_per_second") != nullptr) it->requestRatePerSecond = numberAt(overrideNode, "request_rate_per_second");
            if (overrideNode.find("processing_capacity_per_second") != nullptr) it->processingCapacityPerSecond = numberAt(overrideNode, "processing_capacity_per_second");
            if (overrideNode.find("timeout_seconds") != nullptr) it->timeoutSeconds = numberAt(overrideNode, "timeout_seconds", it->timeoutSeconds);
        }
    }
    if (const Json* requestTypes = object.find("request_types"); requestTypes != nullptr && requestTypes->isObject()) {
        scenario.requestTypes.lightweightShare = numberAt(*requestTypes, "lightweight_share", scenario.requestTypes.lightweightShare);
        scenario.requestTypes.databaseHeavyCacheableShare = numberAt(*requestTypes, "database_heavy_cacheable_share", scenario.requestTypes.databaseHeavyCacheableShare);
        scenario.requestTypes.cacheKeySpace = static_cast<int>(numberAt(*requestTypes, "cache_key_space", scenario.requestTypes.cacheKeySpace));
    }
    if (const Json* cache = object.find("cache"); cache != nullptr && cache->isObject()) {
        scenario.cache.enabled = boolAt(*cache, "enabled", scenario.cache.enabled);
        scenario.cache.maxEntries = static_cast<int>(numberAt(*cache, "max_entries", scenario.cache.maxEntries));
        scenario.cache.ttlSeconds = numberAt(*cache, "ttl_seconds", scenario.cache.ttlSeconds);
    }
    if (const Json* retries = object.find("retries"); retries != nullptr && retries->isObject()) {
        scenario.retries.enabled = boolAt(*retries, "enabled", scenario.retries.enabled);
        scenario.retries.maxRetries = static_cast<int>(numberAt(*retries, "max_retries", scenario.retries.maxRetries));
        scenario.retries.retryDelaySeconds = numberAt(*retries, "retry_delay_seconds", scenario.retries.retryDelaySeconds);
    }
    if (const Json* bursts = object.find("bursts"); bursts != nullptr && bursts->isObject()) {
        scenario.bursts = parseBurst(*bursts);
    }
    scenario.requestTimeoutSeconds = numberAt(object, "request_timeout_seconds", scenario.requestTimeoutSeconds);
}

std::vector<ScenarioPhase> parsePhases(const Json& object)
{
    std::vector<ScenarioPhase> phases;
    if (const Json* array = object.find("phases"); array != nullptr && array->isArray()) {
        for (const auto& entry : array->asArray()) {
            ScenarioPhase phase;
            phase.name = stringAt(entry, "display_name", stringAt(entry, "id"));
            phase.eventMessage = stringAt(entry, "event_message");
            phase.startTimeSeconds = numberAt(entry, "start_time_seconds");
            phase.durationSeconds = numberAt(entry, "duration_seconds", 30.0);
            phase.trafficMultiplier = numberAt(entry, "traffic_multiplier", 1.0);
            if (const Json* burst = entry.find("burst_override"); burst != nullptr && burst->isObject()) {
                phase.burstOverride = parseBurst(*burst);
            }
            phase.unlockMechanics = mappedStrings<MechanicType>(entry, "unlock_interventions", mechanicFromId);
            phases.push_back(std::move(phase));
        }
    }
    return phases;
}

InterventionDefinition parseIntervention(const Json& object)
{
    InterventionDefinition intervention;
    intervention.id = stringAt(object, "id");
    intervention.displayName = stringAt(object, "display_name", intervention.id);
    intervention.description = stringAt(object, "description");
    intervention.expectedBenefits = stringAt(object, "expected_benefits");
    intervention.tradeoffs = stringAt(object, "tradeoffs");
    intervention.tags = stringsAt(object, "tags");
    intervention.kind = stringAt(object, "kind") == "topology_mutation" ? InterventionKind::TopologyMutation : InterventionKind::Mechanic;
    intervention.mechanic = mechanicFromId(stringAt(object, "mechanic"));
    intervention.mutation = mutationFromId(stringAt(object, "mutation"));
    intervention.requiresConfirmation = boolAt(object, "requires_confirmation", intervention.kind == InterventionKind::TopologyMutation);
    return intervention;
}

std::vector<Json> loadDirectoryObjects(const std::filesystem::path& directory, ContentLoadResult& result)
{
    std::vector<Json> objects;
    if (!std::filesystem::exists(directory)) {
        result.errors.push_back("Missing content folder: " + directory.string());
        return objects;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        Json json = loadJsonFile(entry.path(), result);
        if (json.isArray()) {
            for (const auto& item : json.asArray()) {
                objects.push_back(item);
            }
        } else if (json.isObject()) {
            objects.push_back(json);
        }
    }
    return objects;
}

void requireIdSet(const std::string& domain, const std::vector<std::string>& ids, ContentLoadResult& result)
{
    std::set<std::string> seen;
    for (const auto& id : ids) {
        if (id.empty()) {
            result.errors.push_back(domain + " entry is missing required id.");
            continue;
        }
        if (!seen.insert(id).second) {
            result.errors.push_back(domain + " has duplicate id: " + id);
        }
    }
}
}

ContentRegistry& ContentRegistry::instance()
{
    static ContentRegistry registry;
    return registry;
}

ContentLoadResult ContentRegistry::loadFromDisk(const std::filesystem::path& root)
{
    ContentLoadResult result = loadInternal(root);
    if (!result.errors.empty() || scenarios_.empty() || progressionTiers_.empty()) {
        loadErrors_ = result.errors;
        loadFallbackContent();
        result.loaded = false;
        return result;
    }
    validate(result);
    if (!result.errors.empty()) {
        loadErrors_ = result.errors;
        loadFallbackContent();
        result.loaded = false;
        return result;
    }
    loadedFromContent_ = true;
    loadErrors_.clear();
    result.loaded = true;
    return result;
}

ContentLoadResult ContentRegistry::loadInternal(const std::filesystem::path& root)
{
    ContentLoadResult result;
    progressionTiers_.clear();
    scenarios_.clear();
    interventions_.clear();

    std::unordered_map<std::string, Json> topologies;
    for (const auto& object : loadDirectoryObjects(root / "topology", result)) {
        if (const Json* nodes = object.find("nodes"); nodes != nullptr && nodes->isArray()) {
            for (const auto& node : nodes->asArray()) {
                const std::string type = stringAt(node, "type");
                if (!knownNodeTypeId(type)) {
                    result.errors.push_back("Topology " + stringAt(object, "id") + " has invalid node type: " + type);
                }
            }
        }
        topologies[stringAt(object, "id")] = object;
    }

    std::unordered_map<std::string, ScenarioObjective> objectives;
    for (const auto& object : loadDirectoryObjects(root / "objectives", result)) {
        auto parsed = parseObjective(object);
        objectives[parsed.id] = std::move(parsed);
    }

    std::unordered_map<std::string, EventDefinition> events;
    for (const auto& object : loadDirectoryObjects(root / "events", result)) {
        if (const Json* trigger = object.find("trigger"); trigger != nullptr && trigger->isObject()) {
            const std::string metric = stringAt(*trigger, "metric");
            if (!metric.empty() && !knownMetricId(metric)) {
                result.errors.push_back("Event " + stringAt(object, "id") + " has invalid metric id: " + metric);
            }
        }
        auto parsed = parseEvent(object);
        events[parsed.id] = std::move(parsed);
    }

    std::unordered_map<std::string, TrafficProfile> trafficProfiles;
    for (const auto& object : loadDirectoryObjects(root / "traffic", result)) {
        auto parsed = parseTraffic(object);
        trafficProfiles[parsed.id] = std::move(parsed);
    }

    std::unordered_map<std::string, ScenarioModifierDefinition> modifiers;
    for (const auto& object : loadDirectoryObjects(root / "modifiers", result)) {
        auto parsed = parseModifier(object, events);
        modifiers[parsed.id] = std::move(parsed);
    }

    for (const auto& object : loadDirectoryObjects(root / "progression", result)) {
        for (const auto& mechanic : stringsAt(object, "available_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Progression " + stringAt(object, "id") + " has invalid intervention id: " + mechanic);
            }
        }
        for (const auto& nodeType : stringsAt(object, "allowed_node_types")) {
            if (!knownNodeTypeId(nodeType)) {
                result.errors.push_back("Progression " + stringAt(object, "id") + " has invalid node type id: " + nodeType);
            }
        }
        ProgressionTierDefinition tier;
        tier.id = stringAt(object, "id");
        tier.displayName = stringAt(object, "display_name", tier.id);
        tier.name = tier.displayName;
        tier.description = stringAt(object, "description");
        tier.tags = stringsAt(object, "tags");
        tier.tier = progressionTierFromId(tier.id);
        tier.visibleMetrics = stringsAt(object, "visible_metrics");
        tier.availableMechanics = mappedStrings<MechanicType>(object, "available_interventions", mechanicFromId);
        tier.allowedPressures = mappedStrings<PressureCategory>(object, "allowed_pressures", pressureFromId);
        tier.allowedNodeTypes = mappedStrings<NodeType>(object, "allowed_node_types", nodeTypeFromId);
        progressionTiers_.push_back(std::move(tier));
    }

    for (const auto& object : loadDirectoryObjects(root / "interventions", result)) {
        const std::string mechanic = stringAt(object, "mechanic");
        if (!knownMechanicId(mechanic)) {
            result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid mechanic id: " + mechanic);
        }
        const std::string mutation = stringAt(object, "mutation");
        if (!mutation.empty() && !knownMutationId(mutation)) {
            result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid mutation id: " + mutation);
        }
        interventions_.push_back(parseIntervention(object));
    }

    for (const auto& object : loadDirectoryObjects(root / "scenarios", result)) {
        ScenarioDefinition scenario;
        scenario.id = stringAt(object, "id");
        scenario.displayName = stringAt(object, "display_name", scenario.id);
        scenario.name = scenario.displayName;
        scenario.description = stringAt(object, "description");
        scenario.tags = stringsAt(object, "tags");
        scenario.archetype = archetypeFromId(stringAt(object, "archetype"));
        scenario.minimumTier = progressionTierFromId(stringAt(object, "minimum_tier"));
        scenario.topologyTemplateId = stringAt(object, "topology_template");
        scenario.educationalFocus = mappedStrings<EducationalFocus>(object, "educational_focus", focusFromId);
        scenario.guaranteedPressures = mappedStrings<PressureCategory>(object, "guaranteed_pressures", pressureFromId);
        for (const auto& mechanic : stringsAt(object, "allowed_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid allowed intervention id: " + mechanic);
            }
        }
        for (const auto& mechanic : stringsAt(object, "recommended_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid recommended intervention id: " + mechanic);
            }
        }
        scenario.allowedMechanics = mappedStrings<MechanicType>(object, "allowed_interventions", mechanicFromId);
        scenario.recommendedMechanics = mappedStrings<MechanicType>(object, "recommended_interventions", mechanicFromId);

        if (const auto topology = topologies.find(scenario.topologyTemplateId); topology != topologies.end()) {
            scenario.nodes = parseNodes(topology->second);
            scenario.links = parseLinks(topology->second, scenario.nodes);
        } else {
            result.errors.push_back("Scenario " + scenario.id + " references missing topology template: " + scenario.topologyTemplateId);
        }
        const std::string trafficId = stringAt(object, "traffic_profile");
        if (const auto traffic = trafficProfiles.find(trafficId); traffic != trafficProfiles.end()) {
            scenario.trafficProfile = traffic->second;
        } else {
            result.errors.push_back("Scenario " + scenario.id + " references missing traffic profile: " + trafficId);
        }
        for (const auto& id : stringsAt(object, "objectives")) {
            if (const auto it = objectives.find(id); it != objectives.end()) scenario.objectives.push_back(it->second);
            else result.errors.push_back("Scenario " + scenario.id + " references missing objective: " + id);
        }
        for (const auto& id : stringsAt(object, "failure_conditions")) {
            if (const auto it = objectives.find(id); it != objectives.end()) scenario.failureConditions.push_back(it->second);
            else result.errors.push_back("Scenario " + scenario.id + " references missing failure condition: " + id);
        }
        for (const auto& id : stringsAt(object, "events")) {
            if (const auto it = events.find(id); it != events.end()) scenario.events.push_back(it->second);
            else result.errors.push_back("Scenario " + scenario.id + " references missing event: " + id);
        }
        for (const auto& id : stringsAt(object, "modifiers")) {
            if (const auto it = modifiers.find(id); it != modifiers.end()) scenario.optionalModifiers.push_back(it->second);
            else result.errors.push_back("Scenario " + scenario.id + " references missing modifier: " + id);
        }
        scenario.phases = parsePhases(object);
        applyScenarioOverrides(scenario, object);
        scenarios_.push_back(std::move(scenario));
    }

    return result;
}

void ContentRegistry::validate(ContentLoadResult& result) const
{
    std::vector<std::string> scenarioIds;
    for (const auto& scenario : scenarios_) {
        scenarioIds.push_back(scenario.id);
        if (scenario.nodes.empty()) result.errors.push_back("Scenario " + scenario.id + " has no topology nodes.");
        if (scenario.links.empty()) result.errors.push_back("Scenario " + scenario.id + " has no topology links.");
        if (scenario.objectives.empty()) result.errors.push_back("Scenario " + scenario.id + " has no objectives.");
        if (scenario.trafficProfile.baseMultiplier < 0.0) result.errors.push_back("Scenario " + scenario.id + " has invalid traffic multiplier.");
        for (const auto& node : scenario.nodes) {
            if (node.requestRatePerSecond < 0.0 || node.processingCapacityPerSecond < 0.0) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid node numeric ranges.");
            }
        }
    }
    requireIdSet("Scenario", scenarioIds, result);

    std::vector<std::string> tierIds;
    for (const auto& tier : progressionTiers_) tierIds.push_back(tier.id);
    requireIdSet("Progression", tierIds, result);

    std::vector<std::string> interventionIds;
    for (const auto& intervention : interventions_) interventionIds.push_back(intervention.id);
    requireIdSet("Intervention", interventionIds, result);
}

void ContentRegistry::loadFallbackContent()
{
    loadedFromContent_ = false;
    progressionTiers_ = {{
        .id = "fallback",
        .displayName = "Fallback",
        .description = "Minimal fallback progression.",
        .tier = ProgressionTier::Foundations,
        .name = "Fallback",
        .visibleMetrics = {"Input rate", "Queue depth", "Latency"},
        .availableMechanics = {MechanicType::ScaleUp, MechanicType::ThrottleTraffic},
        .allowedPressures = {PressureCategory::TrafficPressure, PressureCategory::QueuePressure},
        .allowedNodeTypes = {NodeType::ClientCluster, NodeType::ApiService},
    }};
    ScenarioDefinition scenario;
    scenario.id = "fallback_scenario";
    scenario.displayName = "Fallback Scenario";
    scenario.name = scenario.displayName;
    scenario.description = "Content failed to load; this tiny scenario keeps the simulation usable.";
    scenario.minimumTier = ProgressionTier::Foundations;
    scenario.allowedMechanics = {MechanicType::ScaleUp, MechanicType::ThrottleTraffic};
    scenario.trafficProfile = {.id = "fallback_constant", .displayName = "Constant", .name = "Constant", .type = TrafficProfileType::Constant, .baseMultiplier = 1.0};
    scenario.nodes = {
        {.id = "clients", .name = "Clients", .type = NodeType::ClientCluster, .requestRatePerSecond = 2.0},
        {.id = "api", .name = "API", .type = NodeType::ApiService, .processingCapacityPerSecond = 4.0},
    };
    scenario.links = {{.sourceNode = 0, .targetNode = 1, .baseLatencySeconds = 0.3, .bandwidthPerSecond = 100.0}};
    scenario.objectives = {{.id = "fallback_survive", .displayName = "Survive", .type = ScenarioObjectiveType::SurviveDuration, .summary = "Keep fallback service running for 60s.", .durationSeconds = 60.0}};
    scenarios_ = {std::move(scenario)};
    interventions_ = {
        {.id = "scale_up", .displayName = "Scale Up", .description = "Increase API service capacity.", .expectedBenefits = "Processing capacity, queue pressure", .tradeoffs = "May not solve downstream bottlenecks.", .mechanic = MechanicType::ScaleUp},
    };
}

const std::vector<ProgressionTierDefinition>& ContentRegistry::progressionTiers() const { return progressionTiers_; }

const ProgressionTierDefinition& ContentRegistry::progressionTier(ProgressionTier tier) const
{
    const auto it = std::find_if(progressionTiers_.begin(), progressionTiers_.end(), [tier](const auto& entry) {
        return entry.tier == tier;
    });
    return it != progressionTiers_.end() ? *it : progressionTiers_.front();
}

const std::vector<ScenarioDefinition>& ContentRegistry::scenarios() const { return scenarios_; }
const ScenarioDefinition& ContentRegistry::defaultScenario() const { return scenarios_.front(); }
const std::vector<InterventionDefinition>& ContentRegistry::interventions() const { return interventions_; }
const std::vector<std::string>& ContentRegistry::loadErrors() const { return loadErrors_; }
bool ContentRegistry::loadedFromContent() const { return loadedFromContent_; }

ContentLoadResult ContentManager::loadDefaultContent()
{
    std::vector<std::filesystem::path> candidates;
#ifdef INFRA_CONTENT_DIR
    candidates.emplace_back(INFRA_CONTENT_DIR);
#endif
    candidates.emplace_back("content");
    candidates.emplace_back("../content");
    candidates.emplace_back("../../content");
    candidates.emplace_back("../../../content");

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return ContentRegistry::instance().loadFromDisk(candidate);
        }
    }
    ContentLoadResult result;
    result.errors.push_back("No content directory found.");
    ContentRegistry::instance().loadFallbackContent();
    return result;
}

} // namespace content
