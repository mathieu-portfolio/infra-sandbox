#include "content/ContentRegistry.hpp"

#include "core/parsing/Json.hpp"
#include "core/validation/ValueSpec.hpp"

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

ContentPackMetadata parsePackMetadata(const Json& object)
{
    return {
        .id = stringAt(object, "id"),
        .displayName = stringAt(object, "display_name"),
        .description = stringAt(object, "description"),
        .version = stringAt(object, "version"),
        .author = stringAt(object, "author"),
        .defaultScenarioId = stringAt(object, "default_scenario_id"),
        .dependsOn = stringsAt(object, "depends_on"),
    };
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

std::size_t sizeAt(const Json& object, const std::string& key, std::size_t fallback)
{
    if (const Json* value = object.find(key); value != nullptr && value->isNumber()) {
        return static_cast<std::size_t>(std::max(0.0, value->asNumber()));
    }
    return fallback;
}

std::vector<std::string> stringsAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey)
{
    auto values = stringsAt(object, preferredKey);
    if (!values.empty() || object.find(preferredKey) != nullptr) {
        return values;
    }
    return stringsAt(object, legacyKey);
}

template <typename T, typename F>
std::vector<T> mappedStringsAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey, F mapper)
{
    std::vector<T> values;
    for (const auto& id : stringsAtAny(object, preferredKey, legacyKey)) {
        values.push_back(mapper(id));
    }
    return values;
}

double numberAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey, double fallback = 0.0)
{
    if (const Json* value = object.find(preferredKey); value != nullptr && value->isNumber()) {
        return value->asNumber();
    }
    return numberAt(object, legacyKey, fallback);
}

NumericRange rangeAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey, double fallback = 0.0)
{
    if (object.find(preferredKey) != nullptr) {
        return rangeAt(object, preferredKey, fallback);
    }
    return rangeAt(object, legacyKey, fallback);
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

GameplayDurationUnit durationUnitFromId(const std::string& id)
{
    if (id == "turn" || id == "turns" || id == "operational_cycle" || id == "operational_cycles") return GameplayDurationUnit::Turns;
    if (id == "minute" || id == "minutes") return GameplayDurationUnit::Minutes;
    if (id == "day" || id == "days") return GameplayDurationUnit::Days;
    if (id == "month" || id == "months") return GameplayDurationUnit::Months;
    if (id == "year" || id == "years") return GameplayDurationUnit::Years;
    return GameplayDurationUnit::Seconds;
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
    if (id == "microservice") return NodeType::Microservice;
    if (id == "worker") return NodeType::Worker;
    if (id == "batch_processor") return NodeType::BatchProcessor;
    if (id == "stream_processor") return NodeType::StreamProcessor;
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
        "client_cluster", "api_service", "microservice", "worker", "batch_processor", "stream_processor",
        "database", "cache", "read_replica", "queue", "cdn_edge", "load_balancer",
    };
    return ids.contains(id);
}

EngineeringDomain engineeringDomainFromId(const std::string& id)
{
    if (id == "frontend") return EngineeringDomain::Frontend;
    if (id == "infrastructure" || id == "infra") return EngineeringDomain::Infrastructure;
    if (id == "data") return EngineeringDomain::Data;
    return EngineeringDomain::Backend;
}

bool knownEngineeringDomainId(const std::string& id)
{
    static const std::set<std::string> ids{"frontend", "backend", "infrastructure", "infra", "data"};
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

ObjectiveConditionType objectiveConditionFromId(const std::string& id)
{
    if (id == "pressure_detected") return ObjectiveConditionType::PressureDetected;
    if (id == "pressure_below") return ObjectiveConditionType::PressureBelow;
    if (id == "metric_below") return ObjectiveConditionType::MetricBelow;
    if (id == "action_used") return ObjectiveConditionType::ActionUsed;
    return ObjectiveConditionType::SurviveDuration;
}

ObjectiveRewardType objectiveRewardFromId(const std::string& id)
{
    if (id == "unlock_action" || id == "unlock_intervention") return ObjectiveRewardType::UnlockIntervention;
    if (id == "unlock_metric") return ObjectiveRewardType::UnlockMetric;
    if (id == "unlock_overlay") return ObjectiveRewardType::UnlockOverlay;
    if (id == "unlock_scenario_phase") return ObjectiveRewardType::UnlockScenarioPhase;
    if (id == "unlock_scenario") return ObjectiveRewardType::UnlockScenario;
    if (id == "complete_scenario") return ObjectiveRewardType::CompleteScenario;
    return ObjectiveRewardType::EmitFeedback;
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

EventMoment eventMomentFromId(const std::string& id)
{
    if (id == "planning_start" || id == "planning") return EventMoment::PlanningStart;
    return EventMoment::Simulation;
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
    if (id == "add_regional_demand" || id == "new_area_traffic") return EventEffectType::RegionalDemand;
    if (id == "degrade_node_capacity") return EventEffectType::DatabaseSlowdown;
    if (id == "unlock_action" || id == "unlock_intervention") return EventEffectType::MechanicUnlock;
    if (id == "emit_feedback") return EventEffectType::PartialRecovery;
    return EventEffectType::TrafficSpike;
}

EventLocationScope eventLocationScopeFromId(const std::string& id)
{
    if (id == "random_region") return EventLocationScope::RandomRegion;
    if (id == "region") return EventLocationScope::Region;
    if (id == "node_type") return EventLocationScope::NodeType;
    return EventLocationScope::Global;
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

void validateRange(const NumericRange& range, const std::string& label, ContentLoadResult& result)
{
    content::validateRange(range, label, result.errors);
}

void validateBurstRanges(const BurstScenario& burst, const std::string& label, ContentLoadResult& result)
{
    validateRange(burst.multiplierRange, label + " burst multiplier", result);
    validateRange(burst.periodSecondsRange, label + " burst period", result);
    validateRange(burst.durationSecondsRange, label + " burst duration", result);
}

void validateEventRanges(const EventDefinition& event, const std::string& label, ContentLoadResult& result)
{
    validateRange(event.trigger.timeSecondsRange, label + " trigger time_seconds", result);
    validateRange(event.trigger.delaySecondsRange, label + " trigger delay_seconds", result);
    validateRange(event.effect.trafficMultiplierRange, label + " traffic_multiplier", result);
    validateRange(event.effect.burstMultiplierRange, label + " burst_multiplier", result);
    validateRange(event.effect.databaseCapacityMultiplierRange, label + " database_capacity_multiplier", result);
    validateRange(event.effect.latencyMultiplierRange, label + " latency_multiplier", result);
    validateRange(event.effect.retryDelayMultiplierRange, label + " retry_delay_multiplier", result);
    validateRange(event.effect.regionalDemandRatePerSecondRange, label + " regional_demand_rate_per_second", result);
    if (event.effect.databaseHeavyShareRange) {
        validateRange(*event.effect.databaseHeavyShareRange, label + " database_heavy_share", result);
    }
    validateRange(event.durationSecondsRange, label + " duration_seconds", result);
    if (event.trigger.turnNumber < 0 || event.trigger.delayTurns < 0 || event.durationTurns < 0) {
        result.errors.push_back(label + " has invalid negative turn timing.");
    }
    validateRange(event.intensityRange, label + " intensity", result);
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
    BurstScenario burst;
    burst.enabled = boolAt(object, "enabled");
    burst.multiplierRange = rangeAt(object, "multiplier", 1.0);
    burst.multiplier = burst.multiplierRange.min;
    burst.periodSecondsRange = rangeAtAny(object, "period_turns", "period_seconds", 12.0);
    burst.periodSeconds = burst.periodSecondsRange.min;
    burst.durationSecondsRange = rangeAtAny(object, "duration_turns", "duration_seconds", 3.0);
    burst.durationSeconds = burst.durationSecondsRange.min;
    return burst;
}

GameplayDuration parseGameplayDuration(const Json& object, const std::string& key, GameplayDuration fallback = {})
{
    const Json* duration = object.find(key);
    if (duration == nullptr || !duration->isObject()) {
        return fallback;
    }
    GameplayDuration parsed = fallback;
    parsed.value = numberAt(*duration, "value", parsed.value);
    parsed.unit = durationUnitFromId(stringAt(*duration, "unit", gameplayDurationUnitName(parsed.unit)));
    parsed.label = stringAt(*duration, "label", parsed.label);
    parsed.simulationSeconds = numberAtAny(*duration, "simulation_turn_seconds", "simulation_seconds", parsed.simulationSeconds);
    parsed.advancesCalendar = boolAt(*duration, "advance_calendar", parsed.advancesCalendar);
    if (parsed.label.empty()) {
        parsed.label = std::to_string(static_cast<int>(parsed.value)) + " " + gameplayDurationUnitName(parsed.unit) + " of platform evolution";
    }
    return parsed;
}

std::vector<EngineeringCost> parseEngineeringCosts(const Json& object)
{
    std::vector<EngineeringCost> costs;
    const Json* value = object.find("engineering_costs");
    if (value == nullptr) {
        return costs;
    }
    if (value->isObject()) {
        for (const auto& [domainId, amountJson] : value->asObject()) {
            if (!amountJson.isNumber()) {
                continue;
            }
            const int amount = static_cast<int>(amountJson.asNumber());
            if (amount > 0) {
                costs.push_back({engineeringDomainFromId(domainId), amount});
            }
        }
    } else if (value->isArray()) {
        for (const auto& entry : value->asArray()) {
            const int amount = static_cast<int>(numberAt(entry, "amount"));
            if (amount > 0) {
                costs.push_back({engineeringDomainFromId(stringAt(entry, "domain")), amount});
            }
        }
    }
    return costs;
}

EngineeringCapacity parseEngineeringCapacity(const Json& object, EngineeringCapacity fallback = {})
{
    const Json* value = object.find("engineering_capacity");
    if (value == nullptr || !value->isObject()) {
        return fallback;
    }
    EngineeringCapacity capacity = fallback;
    capacity.frontend = static_cast<int>(numberAt(*value, "frontend", capacity.frontend));
    capacity.backend = static_cast<int>(numberAt(*value, "backend", capacity.backend));
    capacity.infrastructure = static_cast<int>(numberAt(*value, "infrastructure", numberAt(*value, "infra", capacity.infrastructure)));
    capacity.data = static_cast<int>(numberAt(*value, "data", capacity.data));
    capacity.total = static_cast<int>(numberAt(*value, "total", capacity.total));
    return capacity;
}

EngineeringCapacity parseCapacityBonus(const Json& object)
{
    EngineeringCapacity bonus;
    bonus.frontend = 0;
    bonus.backend = 0;
    bonus.infrastructure = 0;
    bonus.data = 0;
    bonus.total = 0;
    if (const Json* value = object.find("capacity_bonus"); value != nullptr && value->isObject()) {
        bonus.frontend = static_cast<int>(numberAt(*value, "frontend"));
        bonus.backend = static_cast<int>(numberAt(*value, "backend"));
        bonus.infrastructure = static_cast<int>(numberAt(*value, "infrastructure", numberAt(*value, "infra")));
        bonus.data = static_cast<int>(numberAt(*value, "data"));
        bonus.total = static_cast<int>(numberAt(*value, "total"));
    }
    return bonus;
}

PressureState parsePressureEffect(const Json& object)
{
    PressureState effect;
    const Json* effects = object.find("pressure_effects");
    if (effects == nullptr || !effects->isObject()) {
        return effect;
    }
    if (const Json* frontend = effects->find("frontend"); frontend != nullptr && frontend->isObject()) {
        effect.frontend.assetWeight = numberAt(*frontend, "asset_weight");
        effect.frontend.renderComplexity = numberAt(*frontend, "render_complexity");
        effect.frontend.cacheEfficiency = numberAt(*frontend, "cache_efficiency");
        effect.frontend.realtimeIntensity = numberAt(*frontend, "realtime_intensity");
        effect.frontend.sessionPersistence = numberAt(*frontend, "session_persistence");
        effect.frontend.mobileCompatibility = numberAt(*frontend, "mobile_compatibility");
    }
    if (const Json* backend = effects->find("backend"); backend != nullptr && backend->isObject()) {
        effect.backend.requestLoad = numberAt(*backend, "request_load");
        effect.backend.queuePressure = numberAt(*backend, "queue_pressure");
        effect.backend.computeIntensity = numberAt(*backend, "compute_intensity");
        effect.backend.serviceFragmentation = numberAt(*backend, "service_fragmentation");
    }
    if (const Json* network = effects->find("network"); network != nullptr && network->isObject()) {
        effect.network.bandwidthPressure = numberAt(*network, "bandwidth_pressure");
        effect.network.latencySensitivity = numberAt(*network, "latency_sensitivity");
        effect.network.trafficBurstiness = numberAt(*network, "traffic_burstiness");
    }
    return effect;
}

void parseCapacityBonusRanges(const Json& object, WorldActionDefinition& action)
{
    if (const Json* value = object.find("capacity_bonus"); value != nullptr && value->isObject()) {
        action.frontendCapacityBonusRange = rangeAt(*value, "frontend", 0.0);
        action.backendCapacityBonusRange = rangeAt(*value, "backend", 0.0);
        action.infrastructureCapacityBonusRange = rangeAt(*value, "infrastructure", numberAt(*value, "infra"));
        if (const Json* infra = value->find("infra"); infra != nullptr && value->find("infrastructure") == nullptr) {
            action.infrastructureCapacityBonusRange = rangeFromJson(*infra, 0.0);
        }
        action.dataCapacityBonusRange = rangeAt(*value, "data", 0.0);
        action.totalCapacityBonusRange = rangeAt(*value, "total", 0.0);
    }
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
    event.moment = eventMomentFromId(stringAt(object, "moment", "simulation"));
    event.weight = numberAt(object, "weight", event.weight);
    if (const Json* location = object.find("location"); location != nullptr && location->isObject()) {
        event.location.scope = eventLocationScopeFromId(stringAt(*location, "scope", "global"));
        event.location.region = stringAt(*location, "region");
        event.location.candidateRegions = stringsAt(*location, "regions");
        event.location.nodeType = nodeTypeFromId(stringAt(*location, "node_type"));
    }
    if (const Json* trigger = object.find("trigger")) {
        event.trigger.type = triggerTypeFromId(stringAt(*trigger, "type"));
        event.trigger.timeSecondsRange = rangeAt(*trigger, "time_seconds", event.trigger.timeSeconds);
        event.trigger.timeSeconds = event.trigger.timeSecondsRange.min;
        event.trigger.turnNumber = static_cast<int>(numberAt(*trigger, "turn", numberAt(*trigger, "turn_number", 0.0)));
        event.trigger.metric = metricFromId(stringAt(*trigger, "metric"));
        event.trigger.pressure = pressureFromId(stringAt(*trigger, "pressure"));
        event.trigger.threshold = numberAt(*trigger, "threshold");
        event.trigger.phaseIndex = static_cast<int>(numberAt(*trigger, "phase_index", -1.0));
        event.trigger.delaySecondsRange = rangeAt(*trigger, "delay_seconds", event.trigger.delaySeconds);
        event.trigger.delaySeconds = event.trigger.delaySecondsRange.min;
        event.trigger.delayTurns = static_cast<int>(numberAt(*trigger, "delay_turns", 0.0));
    }
    if (const Json* effect = object.find("effect")) {
        event.effect.type = effectTypeFromId(stringAt(*effect, "type"));
        event.effect.trafficMultiplierRange = rangeAt(*effect, "traffic_multiplier", event.effect.trafficMultiplier);
        event.effect.trafficMultiplier = event.effect.trafficMultiplierRange.min;
        event.effect.burstMultiplierRange = rangeAt(*effect, "burst_multiplier", event.effect.burstMultiplier);
        event.effect.burstMultiplier = event.effect.burstMultiplierRange.min;
        event.effect.databaseCapacityMultiplierRange = rangeAt(*effect, "database_capacity_multiplier", event.effect.databaseCapacityMultiplier);
        event.effect.databaseCapacityMultiplier = event.effect.databaseCapacityMultiplierRange.min;
        event.effect.latencyMultiplierRange = rangeAt(*effect, "latency_multiplier", event.effect.latencyMultiplier);
        event.effect.latencyMultiplier = event.effect.latencyMultiplierRange.min;
        event.effect.retryDelayMultiplierRange = rangeAt(*effect, "retry_delay_multiplier", event.effect.retryDelayMultiplier);
        event.effect.retryDelayMultiplier = event.effect.retryDelayMultiplierRange.min;
        event.effect.regionalDemandRatePerSecondRange = rangeAt(*effect, "regional_demand_rate_per_second", event.effect.regionalDemandRatePerSecond);
        event.effect.regionalDemandRatePerSecond = event.effect.regionalDemandRatePerSecondRange.min;
        if (const Json* share = effect->find("database_heavy_share"); share != nullptr) {
            event.effect.databaseHeavyShareRange = rangeFromJson(*share, 0.0);
            event.effect.databaseHeavyShare = event.effect.databaseHeavyShareRange->min;
        }
        event.effect.unlockMechanics = mappedStringsAny<MechanicType>(*effect, "unlock_actions", "unlock_interventions", mechanicFromId);
    }
    event.durationSecondsRange = rangeAtAny(object, "duration_turns", "duration_seconds", event.durationSeconds);
    event.durationSeconds = event.durationSecondsRange.min;
    event.durationTurns = static_cast<int>(numberAt(object, "duration_turns", event.durationTurns > 0 ? static_cast<double>(event.durationTurns) : event.durationSecondsRange.min));
    event.intensityRange = rangeAt(object, "intensity", event.intensity);
    event.intensity = event.intensityRange.min;
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
    objective.conditionType = objectiveConditionFromId(stringAt(object, "condition", stringAt(object, "type")));
    objective.conditionMetric = stringAt(object, "metric");
    objective.pressure = pressureFromId(stringAt(object, "pressure"));
    objective.targetNodeId = stringAt(object, "target_node");
    objective.summary = stringAt(object, "summary", objective.displayName);
    objective.threshold = numberAt(object, "threshold");
    objective.durationTurns = static_cast<int>(numberAt(object, "duration_turns", 0.0));
    objective.durationSeconds = numberAt(object, "duration_seconds", static_cast<double>(objective.durationTurns));
    objective.startsActive = boolAt(object, "starts_active");
    if (const Json* rewards = object.find("rewards"); rewards != nullptr && rewards->isArray()) {
        for (const auto& rewardJson : rewards->asArray()) {
            ObjectiveReward reward;
            reward.type = objectiveRewardFromId(stringAt(rewardJson, "type"));
            reward.id = stringAt(rewardJson, "id");
            reward.message = stringAt(rewardJson, "message");
            objective.rewards.push_back(std::move(reward));
        }
    }
    objective.nextObjectives = stringsAt(object, "next_objectives");
    return objective;
}

ScenarioObjective objectiveFromScenarioEntry(const Json& entry, const std::unordered_map<std::string, ScenarioObjective>& objectives, ContentLoadResult& result, const std::string& scenarioId)
{
    if (entry.isString()) {
        if (const auto it = objectives.find(entry.asString()); it != objectives.end()) {
            return it->second;
        }
        result.errors.push_back("Scenario " + scenarioId + " references missing objective: " + entry.asString());
        return {};
    }
    const std::string id = stringAt(entry, "objective_id", stringAt(entry, "id"));
    ScenarioObjective objective;
    if (const auto it = objectives.find(id); it != objectives.end()) {
        objective = it->second;
    } else {
        result.errors.push_back("Scenario " + scenarioId + " references missing objective: " + id);
        objective.id = id;
    }
    if (entry.find("starts_active") != nullptr) objective.startsActive = boolAt(entry, "starts_active");
    if (entry.find("next_objectives") != nullptr) objective.nextObjectives = stringsAt(entry, "next_objectives");
    if (const Json* rewards = entry.find("rewards"); rewards != nullptr && rewards->isArray()) {
        objective.rewards.clear();
        for (const auto& rewardJson : rewards->asArray()) {
            ObjectiveReward reward;
            reward.type = objectiveRewardFromId(stringAt(rewardJson, "type"));
            reward.id = stringAt(rewardJson, "id");
            reward.message = stringAt(rewardJson, "message");
            objective.rewards.push_back(std::move(reward));
        }
    }
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
    profile.baseMultiplierRange = rangeAt(object, "base_multiplier", profile.baseMultiplier);
    profile.baseMultiplier = profile.baseMultiplierRange.min;
    profile.growthPerSecondRange = object.find("growth_per_turn") != nullptr
        ? rangeAt(object, "growth_per_turn", profile.growthPerSecond)
        : rangeAt(object, "growth_per_second", profile.growthPerSecond);
    profile.growthPerSecond = profile.growthPerSecondRange.min;
    if (const Json* evolution = object.find("evolution"); evolution != nullptr && evolution->isObject()) {
        profile.evolution.enabled = boolAt(*evolution, "enabled", true);
        profile.evolution.pressureSensitivity = numberAt(*evolution, "pressure_sensitivity", profile.evolution.pressureSensitivity);
        profile.evolution.churnSensitivity = numberAt(*evolution, "churn_sensitivity", profile.evolution.churnSensitivity);
        profile.evolution.migrationSensitivity = numberAt(*evolution, "migration_sensitivity", profile.evolution.migrationSensitivity);
        profile.evolution.reroutePressureSensitivity = numberAt(*evolution, "reroute_pressure_sensitivity", profile.evolution.reroutePressureSensitivity);
        profile.evolution.rerouteLatencySensitivity = numberAt(*evolution, "reroute_latency_sensitivity", profile.evolution.rerouteLatencySensitivity);
        profile.evolution.dynamicRetrySensitivity = numberAt(*evolution, "dynamic_retry_sensitivity", profile.evolution.dynamicRetrySensitivity);
        profile.evolution.burstAmplification = numberAt(*evolution, "burst_amplification", profile.evolution.burstAmplification);
    }
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
    modifier.selectionWeightRange = rangeAt(object, "selection_weight", modifier.selectionWeight);
    modifier.selectionWeight = modifier.selectionWeightRange.min;
    modifier.trafficMultiplierRange = rangeAt(object, "traffic_multiplier", modifier.trafficMultiplier);
    modifier.trafficMultiplier = modifier.trafficMultiplierRange.min;
    if (const Json* share = object.find("database_heavy_share"); share != nullptr) {
        modifier.databaseHeavyShareRange = rangeFromJson(*share, 0.0);
        modifier.databaseHeavyShare = modifier.databaseHeavyShareRange->min;
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
            if (const Json* profile = entry.find("resource_profile"); profile != nullptr && profile->isObject()) {
                node.resourceProfile.compute = numberAt(*profile, "compute", node.resourceProfile.compute);
                node.resourceProfile.memory = numberAt(*profile, "memory", node.resourceProfile.memory);
                node.resourceProfile.storage = numberAt(*profile, "storage", node.resourceProfile.storage);
                node.resourceProfile.network = numberAt(*profile, "network", node.resourceProfile.network);
            }
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
            if (const Json* geo = overrideNode.find("geo"); geo != nullptr && geo->isObject()) {
                it->geoLocation = parseGeo(*geo);
            }
        }
    }
    scenario.proceduralLocations = boolAt(object, "procedural_locations", scenario.proceduralLocations);
    scenario.proceduralLocationJitterDegrees = numberAt(object, "procedural_location_jitter_degrees", scenario.proceduralLocationJitterDegrees);
    if (const Json* regions = object.find("procedural_location_regions"); regions != nullptr && regions->isObject()) {
        for (const auto& [nodeId, regionList] : regions->asObject()) {
            if (!regionList.isArray()) {
                continue;
            }
            auto& destinations = scenario.proceduralLocationRegions[nodeId];
            destinations.clear();
            for (const auto& region : regionList.asArray()) {
                if (region.isString()) {
                    destinations.push_back(region.asString());
                }
            }
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
            phase.startTimeSecondsRange = rangeAt(entry, "start_time_seconds", phase.startTimeSeconds);
            phase.startTimeSeconds = phase.startTimeSecondsRange.min;
            phase.startTurn = static_cast<int>(numberAt(entry, "start_turn", 0.0));
            phase.durationSecondsRange = rangeAtAny(entry, "duration_turns", "duration_seconds", phase.durationSeconds);
            phase.durationSeconds = phase.durationSecondsRange.min;
            phase.durationTurns = static_cast<int>(numberAt(entry, "duration_turns", 0.0));
            phase.transitionDuration = parseGameplayDuration(entry, "transition_duration", phase.transitionDuration);
            phase.trafficMultiplierRange = rangeAt(entry, "traffic_multiplier", phase.trafficMultiplier);
            phase.trafficMultiplier = phase.trafficMultiplierRange.min;
            if (const Json* burst = entry.find("burst_override"); burst != nullptr && burst->isObject()) {
                phase.burstOverride = parseBurst(*burst);
            }
            phase.unlockMechanics = mappedStringsAny<MechanicType>(entry, "unlock_actions", "unlock_interventions", mechanicFromId);
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
    intervention.positiveEffects = stringsAt(object, "positive_effects");
    intervention.negativeEffects = stringsAt(object, "negative_effects");
    intervention.pressureShifts = stringsAt(object, "pressure_shifts");
    intervention.categories = stringsAt(object, "categories");
    intervention.usefulWhen = stringsAt(object, "useful_when");
    intervention.showUsageDetails = boolAt(object, "show_usage_details", true);
    intervention.iconId = stringAt(object, "icon_id");
    intervention.affectedPressures = mappedStrings<PressureCategory>(object, "affected_pressures", pressureFromId);
    intervention.targetNodeTypes = mappedStrings<NodeType>(object, "node_types", nodeTypeFromId);
    intervention.engineeringCosts = parseEngineeringCosts(object);
    intervention.architecturalPattern = stringAt(object, "architectural_pattern");
    intervention.technologyExample = stringAt(object, "technology_example");
    intervention.tags = stringsAt(object, "tags");
    intervention.kind = stringAt(object, "kind") == "topology_mutation" ? InterventionKind::TopologyMutation : InterventionKind::Mechanic;
    intervention.mechanic = mechanicFromId(stringAt(object, "mechanic"));
    intervention.mutation = mutationFromId(stringAt(object, "mutation"));
    intervention.requiresConfirmation = boolAt(object, "requires_confirmation", intervention.kind == InterventionKind::TopologyMutation);
    intervention.complexityCost = numberAt(object, "complexity_cost");
    intervention.maxScaleLevel = static_cast<int>(numberAt(object, "max_scale_level", intervention.maxScaleLevel));
    intervention.diminishingReturn = numberAt(object, "diminishing_return", intervention.diminishingReturn);
    intervention.regionSlotUsage = static_cast<int>(numberAt(object, "region_slot_usage", intervention.kind == InterventionKind::TopologyMutation ? 1.0 : 0.0));
    intervention.useLimit = static_cast<int>(numberAt(object, "use_limit", intervention.useLimit));
    intervention.pressureEffect = parsePressureEffect(object);
    return intervention;
}

WorldActionDefinition parseWorldAction(const Json& object)
{
    WorldActionDefinition action;
    action.id = stringAt(object, "id");
    action.displayName = stringAt(object, "display_name", action.id);
    action.description = stringAt(object, "description");
    action.tags = stringsAt(object, "tags");
    action.categories = stringsAt(object, "categories");
    action.usefulWhen = stringAt(object, "useful_when");
    action.tradeoffs = stringAt(object, "tradeoffs");
    action.showUsageDetails = boolAt(object, "show_usage_details", true);
    action.iconId = stringAt(object, "icon_id", "action.generic");
    action.affectedPressures = mappedStrings<PressureCategory>(object, "affected_pressures", pressureFromId);
    action.capacityBonus = parseCapacityBonus(object);
    parseCapacityBonusRanges(object, action);
    action.pressureResistanceRange = rangeAt(object, "pressure_resistance", action.pressureResistance);
    action.pressureResistance = action.pressureResistanceRange.min;
    action.eventIntensityMultiplierRange = rangeAt(object, "event_intensity_multiplier", action.eventIntensityMultiplier);
    action.eventIntensityMultiplier = action.eventIntensityMultiplierRange.min;
    action.complexityDeltaRange = rangeAt(object, "complexity_delta", action.complexityDelta);
    action.complexityDelta = action.complexityDeltaRange.min;
    action.durationSecondsRange = rangeAtAny(object, "duration_turns", "duration_seconds", action.durationSeconds);
    action.durationSeconds = action.durationSecondsRange.min;
    action.unlocksObservability = stringsAtAny(object, "unlocks_observability", "observability_unlocks");
    action.pressureEffect = parsePressureEffect(object);
    if (const Json* intensity = object.find("intensity_range"); intensity != nullptr && intensity->isObject()) {
        action.minIntensity = numberAt(*intensity, "min", action.minIntensity);
        action.maxIntensity = numberAt(*intensity, "max", action.maxIntensity);
    }
    return action;
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


Json substituteTemplateArgs(const Json& value, const Json::Object& args)
{
    if (value.isString()) {
        const std::string& text = value.asString();
        if (text.size() > 1 && text[0] == '$') {
            const auto it = args.find(text.substr(1));
            if (it != args.end()) {
                return it->second;
            }
        }
        return value;
    }
    if (value.isArray()) {
        Json::Array array;
        for (const auto& entry : value.asArray()) {
            array.push_back(substituteTemplateArgs(entry, args));
        }
        return Json(std::move(array));
    }
    if (value.isObject()) {
        Json::Object object;
        for (const auto& [key, entry] : value.asObject()) {
            object[key] = substituteTemplateArgs(entry, args);
        }
        return Json(std::move(object));
    }
    return value;
}

Json deepMergeJson(const Json& base, const Json& overrideValue)
{
    if (!base.isObject() || !overrideValue.isObject()) {
        return overrideValue;
    }
    Json::Object merged = base.asObject();
    for (const auto& [key, value] : overrideValue.asObject()) {
        const auto existing = merged.find(key);
        if (existing != merged.end() && existing->second.isObject() && value.isObject()) {
            existing->second = deepMergeJson(existing->second, value);
        } else {
            merged[key] = value;
        }
    }
    return Json(std::move(merged));
}

Json resolveTemplateObject(
    const Json& object,
    const std::unordered_map<std::string, Json>& templates,
    ContentLoadResult& result,
    std::vector<std::string>& stack)
{
    if (object.isArray()) {
        Json::Array array;
        for (const auto& entry : object.asArray()) {
            array.push_back(resolveTemplateObject(entry, templates, result, stack));
        }
        return Json(std::move(array));
    }
    if (!object.isObject()) {
        return object;
    }

    Json::Object expandedChildren;
    for (const auto& [key, value] : object.asObject()) {
        if (key == "template" || key == "args" || key == "overrides") {
            expandedChildren[key] = value;
        } else {
            expandedChildren[key] = resolveTemplateObject(value, templates, result, stack);
        }
    }

    const std::string templateId = stringAt(object, "template");
    if (templateId.empty()) {
        return Json(std::move(expandedChildren));
    }

    if (std::find(stack.begin(), stack.end(), templateId) != stack.end()) {
        result.errors.push_back("Template cycle detected while resolving: " + templateId);
        return Json(std::move(expandedChildren));
    }
    const auto templateIt = templates.find(templateId);
    if (templateIt == templates.end()) {
        result.errors.push_back("Missing content template: " + templateId);
        return Json(std::move(expandedChildren));
    }

    stack.push_back(templateId);
    Json resolvedTemplate = resolveTemplateObject(templateIt->second, templates, result, stack);
    stack.pop_back();

    Json::Object args;
    if (const Json* argsJson = object.find("args"); argsJson != nullptr && argsJson->isObject()) {
        for (const auto& [key, value] : argsJson->asObject()) {
            args[key] = resolveTemplateObject(value, templates, result, stack);
        }
    }
    resolvedTemplate = substituteTemplateArgs(resolvedTemplate, args);

    Json::Object localOverrides;
    for (const auto& [key, value] : expandedChildren) {
        if (key != "template" && key != "args" && key != "overrides") {
            localOverrides[key] = value;
        }
    }
    Json merged = deepMergeJson(resolvedTemplate, Json(std::move(localOverrides)));
    if (const Json* overrides = object.find("overrides"); overrides != nullptr && overrides->isObject()) {
        merged = deepMergeJson(merged, resolveTemplateObject(*overrides, templates, result, stack));
    }
    return merged;
}

std::vector<Json> resolveTemplateObjects(
    const std::vector<Json>& objects,
    const std::unordered_map<std::string, Json>& templates,
    ContentLoadResult& result)
{
    std::vector<Json> resolved;
    std::vector<std::string> stack;
    for (const auto& object : objects) {
        resolved.push_back(resolveTemplateObject(object, templates, result, stack));
    }
    return resolved;
}

std::unordered_map<std::string, Json> loadLayerTemplates(const std::vector<std::filesystem::path>& roots, ContentLoadResult& result)
{
    std::unordered_map<std::string, Json> templates;
    for (const auto& root : roots) {
        const auto directory = root / "templates";
        if (!std::filesystem::exists(directory)) {
            continue;
        }
        for (const auto& object : loadDirectoryObjects(directory, result)) {
            const std::string id = stringAt(object, "id");
            if (id.empty()) {
                result.errors.push_back("Template object in " + directory.string() + " is missing required id.");
                continue;
            }
            templates[id] = object;
        }
    }
    return templates;
}

std::vector<Json> loadOptionalDirectoryObjects(const std::filesystem::path& directory, ContentLoadResult& result)
{
    if (!std::filesystem::exists(directory)) {
        return {};
    }
    return loadDirectoryObjects(directory, result);
}

std::vector<Json> loadLayerDirectoryObjects(const std::vector<std::filesystem::path>& roots, const std::string& folder, ContentLoadResult& result, bool required = true, const std::unordered_map<std::string, Json>* templates = nullptr)
{
    std::vector<Json> objects;
    bool found = false;
    for (const auto& root : roots) {
        const auto directory = root / folder;
        if (!std::filesystem::exists(directory)) {
            continue;
        }
        found = true;
        auto layerObjects = loadDirectoryObjects(directory, result);
        if (templates != nullptr && !templates->empty()) {
            layerObjects = resolveTemplateObjects(layerObjects, *templates, result);
        }
        objects.insert(objects.end(), layerObjects.begin(), layerObjects.end());
    }
    if (required && !found) {
        result.errors.push_back("Missing content folder in loaded layers: " + folder);
    }
    return objects;
}

void applyPressureAnalysisConfig(SimulationConfig& config, const Json& object)
{
    const Json* root = object.find("pressure_analysis");
    if (root == nullptr || !root->isObject()) {
        return;
    }
    auto& pressure = config.pressureAnalysis;
    if (const Json* thresholds = root->find("thresholds"); thresholds != nullptr && thresholds->isObject()) {
        pressure.retryDominantThreshold = numberAt(*thresholds, "retry_dominant", pressure.retryDominantThreshold);
        pressure.failureDominantThreshold = numberAt(*thresholds, "failure_dominant", pressure.failureDominantThreshold);
        pressure.persistenceQueueThreshold = numberAt(*thresholds, "persistence_queue", pressure.persistenceQueueThreshold);
        pressure.latencyContributionThreshold = numberAt(*thresholds, "latency_contribution", pressure.latencyContributionThreshold);
        pressure.averageLatencyThreshold = numberAt(*thresholds, "average_latency", pressure.averageLatencyThreshold);
        pressure.queuePressureThreshold = numberAt(*thresholds, "queue_pressure", pressure.queuePressureThreshold);
        pressure.queueGrowthThreshold = numberAt(*thresholds, "queue_growth_per_second", pressure.queueGrowthThreshold);
        pressure.computePressureThreshold = numberAt(*thresholds, "compute_pressure", pressure.computePressureThreshold);
        pressure.trafficBacklogThreshold = numberAt(*thresholds, "traffic_backlog", pressure.trafficBacklogThreshold);
        pressure.dependencyPressureThreshold = numberAt(*thresholds, "dependency_pressure", pressure.dependencyPressureThreshold);
        pressure.databaseQueueHintThreshold = numberAt(*thresholds, "database_queue_hint", pressure.databaseQueueHintThreshold);
        pressure.databaseUtilizationHintThreshold = numberAt(*thresholds, "database_utilization_hint", pressure.databaseUtilizationHintThreshold);
        pressure.apiQueueHintThreshold = numberAt(*thresholds, "api_queue_hint", pressure.apiQueueHintThreshold);
        pressure.retryRateHintThreshold = numberAt(*thresholds, "retry_rate_hint", pressure.retryRateHintThreshold);
        pressure.timeoutRateHintThreshold = numberAt(*thresholds, "timeout_rate_hint", pressure.timeoutRateHintThreshold);
        pressure.failureTimeoutRateThreshold = numberAt(*thresholds, "failure_timeout_rate", pressure.failureTimeoutRateThreshold);
        pressure.cacheHitSurgeThreshold = numberAt(*thresholds, "cache_hit_surge", pressure.cacheHitSurgeThreshold);
    }
    if (const Json* weights = root->find("weights"); weights != nullptr && weights->isObject()) {
        pressure.queueCapacityWindow = numberAt(*weights, "queue_capacity_window", pressure.queueCapacityWindow);
        pressure.timeoutRateScale = numberAt(*weights, "timeout_rate_scale", pressure.timeoutRateScale);
        pressure.retryRateScale = numberAt(*weights, "retry_rate_scale", pressure.retryRateScale);
        pressure.databaseRetryFloor = numberAt(*weights, "database_retry_floor", pressure.databaseRetryFloor);
        pressure.processorRetryFloor = numberAt(*weights, "processor_retry_floor", pressure.processorRetryFloor);
    }
    if (const Json* history = root->find("history"); history != nullptr && history->isObject()) {
        pressure.eventCooldownSeconds = numberAtAny(*history, "event_cooldown_turns", "event_cooldown_seconds", pressure.eventCooldownSeconds);
        pressure.pressureHistoryLimit = sizeAt(*history, "pressure_history_limit", pressure.pressureHistoryLimit);
        pressure.recentEventLimit = sizeAt(*history, "recent_event_limit", pressure.recentEventLimit);
        pressure.recurringPressureSampleCount = static_cast<int>(numberAt(*history, "recurring_sample_count", pressure.recurringPressureSampleCount));
        pressure.recurringPressureMinimum = static_cast<int>(numberAt(*history, "recurring_minimum", pressure.recurringPressureMinimum));
    }
    if (const Json* text = root->find("text"); text != nullptr && text->isObject()) {
        pressure.persistenceNodeExplanation = stringAt(*text, "persistence_node_explanation", pressure.persistenceNodeExplanation);
        pressure.retryNodeExplanation = stringAt(*text, "retry_node_explanation", pressure.retryNodeExplanation);
        pressure.geoLatencyExplanation = stringAt(*text, "geo_latency_explanation", pressure.geoLatencyExplanation);
        pressure.localLatencyExplanation = stringAt(*text, "local_latency_explanation", pressure.localLatencyExplanation);
        pressure.queueNodeExplanation = stringAt(*text, "queue_node_explanation", pressure.queueNodeExplanation);
        pressure.computeNodeExplanation = stringAt(*text, "compute_node_explanation", pressure.computeNodeExplanation);
        pressure.failureNodeExplanation = stringAt(*text, "failure_node_explanation", pressure.failureNodeExplanation);
        pressure.trafficNodeExplanation = stringAt(*text, "traffic_node_explanation", pressure.trafficNodeExplanation);
        pressure.processorNoPressureExplanation = stringAt(*text, "processor_no_pressure_explanation", pressure.processorNoPressureExplanation);
        pressure.trafficSourceExplanation = stringAt(*text, "traffic_source_explanation", pressure.trafficSourceExplanation);
        pressure.dependencyPressureSummary = stringAt(*text, "dependency_pressure_summary", pressure.dependencyPressureSummary);
        pressure.dependencyStableSummary = stringAt(*text, "dependency_stable_summary", pressure.dependencyStableSummary);
        pressure.noDependencySummary = stringAt(*text, "no_dependency_summary", pressure.noDependencySummary);
        pressure.databaseQueueHint = stringAt(*text, "database_queue_hint", pressure.databaseQueueHint);
        pressure.databaseLatencyHint = stringAt(*text, "database_latency_hint", pressure.databaseLatencyHint);
        pressure.databaseExplanation = stringAt(*text, "database_explanation", pressure.databaseExplanation);
        pressure.databasePattern = stringAt(*text, "database_pattern", pressure.databasePattern);
        pressure.apiQueueHint = stringAt(*text, "api_queue_hint", pressure.apiQueueHint);
        pressure.apiExplanation = stringAt(*text, "api_explanation", pressure.apiExplanation);
        pressure.apiPattern = stringAt(*text, "api_pattern", pressure.apiPattern);
        pressure.retryHint = stringAt(*text, "retry_hint", pressure.retryHint);
        pressure.retryPattern = stringAt(*text, "retry_pattern", pressure.retryPattern);
        pressure.latencyHint = stringAt(*text, "latency_hint", pressure.latencyHint);
        pressure.latencyExplanation = stringAt(*text, "latency_explanation", pressure.latencyExplanation);
        pressure.cacheHint = stringAt(*text, "cache_hint", pressure.cacheHint);
    }
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

bool isNodeActionObject(const Json& object)
{
    const std::string kind = stringAt(object, "kind");
    return kind == "mechanic" || kind == "topology_mutation" || !stringAt(object, "mechanic").empty() || !stringAt(object, "mutation").empty();
}
}

ContentLoadResult ContentRegistry::loadInternal(const std::filesystem::path& root)
{
    return loadInternal(std::vector<std::filesystem::path>{root});
}

ContentLoadResult ContentRegistry::loadInternal(const std::vector<std::filesystem::path>& roots)
{
    ContentLoadResult result;
    clearLoadedContent();
    if (roots.empty()) {
        result.errors.push_back("No content layers provided.");
        return result;
    }
    const auto& root = roots.back();
    const auto contentTemplates = loadLayerTemplates(roots, result);

    const Json metadata = loadJsonFile(root / "pack.json", result);
    if (metadata.isObject()) {
        packMetadata_ = parsePackMetadata(metadata);
        if (packMetadata_.id.empty()) result.errors.push_back("Content pack is missing required id.");
        if (packMetadata_.displayName.empty()) result.errors.push_back("Content pack " + packMetadata_.id + " is missing display_name.");
        if (packMetadata_.version.empty()) result.errors.push_back("Content pack " + packMetadata_.id + " is missing version.");
    }

    for (const auto& object : loadLayerDirectoryObjects(roots, "balancing", result, false, &contentTemplates)) {
        applyPressureAnalysisConfig(simulationConfig_, object);
    }

    std::unordered_map<std::string, Json> topologies;
    for (const auto& object : loadLayerDirectoryObjects(roots, "topology", result, true, &contentTemplates)) {
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
    for (const auto& object : loadLayerDirectoryObjects(roots, "objectives", result, true, &contentTemplates)) {
        auto parsed = parseObjective(object);
        objectives[parsed.id] = std::move(parsed);
    }

    std::unordered_map<std::string, EventDefinition> events;
    for (const auto& object : loadLayerDirectoryObjects(roots, "events", result, true, &contentTemplates)) {
        if (const Json* location = object.find("location"); location != nullptr && location->isObject()) {
            const std::string scope = stringAt(*location, "scope", "global");
            if (scope != "global" && scope != "region" && scope != "node_type" && scope != "random_region") {
                result.errors.push_back("Event " + stringAt(object, "id") + " has invalid location scope: " + scope);
            }
            if (scope == "region" && stringAt(*location, "region").empty()) {
                result.errors.push_back("Event " + stringAt(object, "id") + " has region scope without region.");
            }
            if (scope == "node_type" && !knownNodeTypeId(stringAt(*location, "node_type"))) {
                result.errors.push_back("Event " + stringAt(object, "id") + " has invalid location node type: " + stringAt(*location, "node_type"));
            }
        }
        if (const Json* trigger = object.find("trigger"); trigger != nullptr && trigger->isObject()) {
            const std::string metric = stringAt(*trigger, "metric");
            if (!metric.empty() && !knownMetricId(metric)) {
                result.errors.push_back("Event " + stringAt(object, "id") + " has invalid metric id: " + metric);
            }
        }
        auto parsed = parseEvent(object);
        validateEventRanges(parsed, "Event " + parsed.id, result);
        events[parsed.id] = std::move(parsed);
    }

    std::unordered_map<std::string, TrafficProfile> trafficProfiles;
    std::vector<Json> trafficObjects = loadLayerDirectoryObjects(roots, "traffic_patterns", result, false, &contentTemplates);
    if (trafficObjects.empty()) {
        trafficObjects = loadLayerDirectoryObjects(roots, "traffic", result, true, &contentTemplates);
    }
    for (const auto& object : trafficObjects) {
        auto parsed = parseTraffic(object);
        validateRange(parsed.baseMultiplierRange, "Traffic profile " + parsed.id + " base_multiplier", result);
        validateRange(parsed.growthPerSecondRange, "Traffic pattern " + parsed.id + " growth_per_turn", result);
        trafficProfiles[parsed.id] = std::move(parsed);
    }

    std::unordered_map<std::string, ScenarioModifierDefinition> modifiers;
    for (const auto& object : loadLayerDirectoryObjects(roots, "modifiers", result, true, &contentTemplates)) {
        auto parsed = parseModifier(object, events);
        validateRange(parsed.selectionWeightRange, "Modifier " + parsed.id + " selection_weight", result);
        validateRange(parsed.trafficMultiplierRange, "Modifier " + parsed.id + " traffic_multiplier", result);
        if (parsed.databaseHeavyShareRange) {
            validateRange(*parsed.databaseHeavyShareRange, "Modifier " + parsed.id + " database_heavy_share", result);
        }
        if (parsed.burstOverride) {
            validateBurstRanges(*parsed.burstOverride, "Modifier " + parsed.id, result);
        }
        for (const auto& event : parsed.events) {
            validateEventRanges(event, "Modifier " + parsed.id + " event " + event.id, result);
        }
        modifiers[parsed.id] = std::move(parsed);
    }

    for (const auto& object : loadLayerDirectoryObjects(roots, "progression", result, true, &contentTemplates)) {
        for (const auto& mechanic : stringsAtAny(object, "available_actions", "available_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Progression " + stringAt(object, "id") + " has invalid action id: " + mechanic);
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
        tier.availableMechanics = mappedStringsAny<MechanicType>(object, "available_actions", "available_interventions", mechanicFromId);
        tier.allowedPressures = mappedStrings<PressureCategory>(object, "allowed_pressures", pressureFromId);
        tier.allowedNodeTypes = mappedStrings<NodeType>(object, "allowed_node_types", nodeTypeFromId);
        progressionTiers_.push_back(std::move(tier));
    }

    for (const auto& object : loadLayerDirectoryObjects(roots, "actions", result, true, &contentTemplates)) {
        if (!isNodeActionObject(object)) {
            continue;
        }
        const std::string mechanic = stringAt(object, "mechanic");
        if (!knownMechanicId(mechanic)) {
            result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid mechanic id: " + mechanic);
        }
        const std::string mutation = stringAt(object, "mutation");
        if (!mutation.empty() && !knownMutationId(mutation)) {
            result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid mutation id: " + mutation);
        }
        if (numberAt(object, "complexity_cost") < 0.0) {
            result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid complexity cost.");
        }
        if (numberAt(object, "max_scale_level", 1.0) < 1.0) {
            result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid max scale level.");
        }
        if (numberAt(object, "region_slot_usage", 0.0) < 0.0) {
            result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid region slot usage.");
        }
        if (const Json* costs = object.find("engineering_costs"); costs != nullptr) {
            if (costs->isObject()) {
                for (const auto& [domainId, amountJson] : costs->asObject()) {
                    if (!knownEngineeringDomainId(domainId)) {
                        result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid engineering domain: " + domainId);
                    }
                    if (!amountJson.isNumber() || amountJson.asNumber() < 0.0) {
                        result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid engineering cost for " + domainId + ".");
                    }
                }
            } else if (costs->isArray()) {
                for (const auto& cost : costs->asArray()) {
                    const std::string domainId = stringAt(cost, "domain");
                    if (!knownEngineeringDomainId(domainId)) {
                        result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid engineering domain: " + domainId);
                    }
                    if (numberAt(cost, "amount") < 0.0) {
                        result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid engineering cost.");
                    }
                }
            } else {
                result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid engineering_costs shape.");
            }
        }
        for (const auto& nodeType : stringsAt(object, "node_types")) {
            if (!knownNodeTypeId(nodeType)) {
                result.errors.push_back("Intervention " + stringAt(object, "id") + " has invalid node type id: " + nodeType);
            }
        }
        interventions_.push_back(parseIntervention(object));
    }

    for (const auto& object : loadLayerDirectoryObjects(roots, "actions", result, false, &contentTemplates)) {
        if (isNodeActionObject(object)) {
            continue;
        }
        WorldActionDefinition action = parseWorldAction(object);
        if (action.minIntensity <= 0.0 || action.maxIntensity < action.minIntensity) {
            result.errors.push_back("World action " + action.id + " has invalid intensity range.");
        }
        validateRange(action.frontendCapacityBonusRange, "World action " + action.id + " frontend capacity_bonus", result);
        validateRange(action.backendCapacityBonusRange, "World action " + action.id + " backend capacity_bonus", result);
        validateRange(action.infrastructureCapacityBonusRange, "World action " + action.id + " infrastructure capacity_bonus", result);
        validateRange(action.dataCapacityBonusRange, "World action " + action.id + " data capacity_bonus", result);
        validateRange(action.totalCapacityBonusRange, "World action " + action.id + " budget capacity_bonus", result);
        validateRange(action.pressureResistanceRange, "World action " + action.id + " pressure_resistance", result);
        validateRange(action.eventIntensityMultiplierRange, "World action " + action.id + " event_intensity_multiplier", result);
        validateRange(action.complexityDeltaRange, "World action " + action.id + " complexity_delta", result);
        validateRange(action.durationSecondsRange, "World action " + action.id + " duration_turns", result);
        worldActions_.push_back(std::move(action));
    }

    for (const auto& object : loadLayerDirectoryObjects(roots, "scenarios", result, true, &contentTemplates)) {
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
        for (const auto& mechanic : stringsAtAny(object, "allowed_actions", "allowed_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid allowed intervention id: " + mechanic);
            }
        }
        for (const auto& mechanic : stringsAtAny(object, "starting_actions", "starting_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid starting intervention id: " + mechanic);
            }
        }
        for (const auto& mechanic : stringsAtAny(object, "unlockable_actions", "unlockable_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid unlockable intervention id: " + mechanic);
            }
        }
        for (const auto& mechanic : stringsAtAny(object, "disabled_actions", "disabled_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid disabled intervention id: " + mechanic);
            }
        }
        for (const auto& mechanic : stringsAtAny(object, "recommended_actions", "recommended_interventions")) {
            if (!knownMechanicId(mechanic)) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid recommended intervention id: " + mechanic);
            }
        }
        scenario.allowedMechanics = mappedStringsAny<MechanicType>(object, "allowed_actions", "allowed_interventions", mechanicFromId);
        scenario.startingInterventions = mappedStringsAny<MechanicType>(object, "starting_actions", "starting_interventions", mechanicFromId);
        scenario.unlockableInterventions = mappedStringsAny<MechanicType>(object, "unlockable_actions", "unlockable_interventions", mechanicFromId);
        scenario.disabledInterventions = mappedStringsAny<MechanicType>(object, "disabled_actions", "disabled_interventions", mechanicFromId);
        scenario.recommendedMechanics = mappedStringsAny<MechanicType>(object, "recommended_actions", "recommended_interventions", mechanicFromId);
        scenario.unlocksScenarios = stringsAt(object, "unlocks_scenarios");
        scenario.requiredCompletedScenarios = stringsAt(object, "required_completed_scenarios");
        scenario.requiredConceptTags = stringsAt(object, "required_concept_tags");
        scenario.sandboxLab = boolAt(object, "sandbox_lab");
        scenario.engineeringCapacity = parseEngineeringCapacity(object, scenario.engineeringCapacity);
        scenario.turnDuration = parseGameplayDuration(object, "turn_duration", scenario.turnDuration);

        if (const auto topology = topologies.find(scenario.topologyTemplateId); topology != topologies.end()) {
            scenario.nodes = parseNodes(topology->second);
            scenario.links = parseLinks(topology->second, scenario.nodes);
        } else {
            result.errors.push_back("Scenario " + scenario.id + " references missing topology template: " + scenario.topologyTemplateId);
        }
        const std::string trafficId = stringAt(object, "traffic_pattern", stringAt(object, "traffic_profile"));
        if (const auto traffic = trafficProfiles.find(trafficId); traffic != trafficProfiles.end()) {
            scenario.trafficProfile = traffic->second;
        } else {
            result.errors.push_back("Scenario " + scenario.id + " references missing traffic pattern: " + trafficId);
        }
        if (const Json* scenarioObjectives = object.find("objectives"); scenarioObjectives != nullptr && scenarioObjectives->isArray()) {
            for (const auto& entry : scenarioObjectives->asArray()) {
                scenario.objectives.push_back(objectiveFromScenarioEntry(entry, objectives, result, scenario.id));
            }
        }
        for (const auto& id : stringsAt(object, "failure_conditions")) {
            if (const auto it = objectives.find(id); it != objectives.end()) scenario.failureConditions.push_back(it->second);
            else result.errors.push_back("Scenario " + scenario.id + " references missing failure condition: " + id);
        }
        for (const auto& id : stringsAt(object, "events")) {
            if (const auto it = events.find(id); it != events.end()) scenario.events.push_back(it->second);
            else result.errors.push_back("Scenario " + scenario.id + " references missing event: " + id);
        }
        for (const auto& id : stringsAt(object, "sandbox_events")) {
            if (const auto it = events.find(id); it != events.end()) scenario.sandboxEvents.push_back(it->second);
            else result.errors.push_back("Scenario " + scenario.id + " references missing sandbox event: " + id);
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


} // namespace content
