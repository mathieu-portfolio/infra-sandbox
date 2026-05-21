#pragma once

#include "content/ContentRegistry.hpp"
#include "content/loading/ContentEnumParsers.hpp"
#include "content/loading/ContentJsonAccess.hpp"
#include "core/parsing/Json.hpp"
#include "core/validation/ValueSpec.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace content::loading {

template <typename T, typename F>
inline std::vector<T> mappedStrings(const Json& object, const std::string& key, F mapper)
{
    std::vector<T> values;
    for (const auto& id : stringsAt(object, key)) {
        values.push_back(mapper(id));
    }
    return values;
}

inline void validateRange(const NumericRange& range, const std::string& label, ContentLoadResult& result)
{
    content::validateRange(range, label, result.errors);
}

inline void validateBurstRanges(const BurstScenario& burst, const std::string& label, ContentLoadResult& result)
{
    validateRange(burst.multiplierRange, label + " burst multiplier", result);
    validateRange(burst.periodSecondsRange, label + " burst period", result);
    validateRange(burst.durationSecondsRange, label + " burst duration", result);
}

inline void validateEventRanges(const EventDefinition& event, const std::string& label, ContentLoadResult& result)
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

inline NetworkIdentity parseIdentity(const Json& object)
{
    return {
        stringAt(object, "hostname"),
        stringAt(object, "ip_address"),
        stringAt(object, "endpoint"),
    };
}

inline GeoLocation parseGeo(const Json& object)
{
    return {
        numberAt(object, "latitude"),
        numberAt(object, "longitude"),
        stringAt(object, "region"),
    };
}

inline BurstScenario parseBurst(const Json& object)
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

inline GameplayDuration parseGameplayDuration(const Json& object, const std::string& key, GameplayDuration fallback = {})
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

inline std::vector<EngineeringCost> parseEngineeringCosts(const Json& object)
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

inline EngineeringCapacity parseEngineeringCapacity(const Json& object, EngineeringCapacity fallback = {})
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

inline EngineeringCapacity parseCapacityBonus(const Json& object)
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

inline MetricContributionDomain pressureDomainFromId(const std::string& id)
{
    if (id == "backend") return MetricContributionDomain::Backend;
    if (id == "network") return MetricContributionDomain::Network;
    if (id == "database" || id == "persistence") return MetricContributionDomain::Database;
    if (id == "runtime") return MetricContributionDomain::Runtime;
    return MetricContributionDomain::Frontend;
}

inline PressureState parsePressureStateObject(const Json& effects)
{
    PressureState effect;
    if (!effects.isObject()) {
        return effect;
    }
    if (const Json* frontend = effects.find("frontend"); frontend != nullptr && frontend->isObject()) {
        effect.frontend.assetWeight = numberAt(*frontend, "asset_weight");
        effect.frontend.renderComplexity = numberAt(*frontend, "render_complexity");
        effect.frontend.cacheEfficiency = numberAt(*frontend, "cache_efficiency");
        effect.frontend.realtimeIntensity = numberAt(*frontend, "realtime_intensity");
        effect.frontend.sessionPersistence = numberAt(*frontend, "session_persistence");
        effect.frontend.mobileCompatibility = numberAt(*frontend, "mobile_compatibility");
    }
    if (const Json* backend = effects.find("backend"); backend != nullptr && backend->isObject()) {
        effect.backend.requestLoad = numberAt(*backend, "request_load");
        effect.backend.queuePressure = numberAt(*backend, "queue_pressure");
        effect.backend.computeIntensity = numberAt(*backend, "compute_intensity");
        effect.backend.serviceFragmentation = numberAt(*backend, "service_fragmentation");
    }
    if (const Json* network = effects.find("network"); network != nullptr && network->isObject()) {
        effect.network.bandwidthPressure = numberAt(*network, "bandwidth_pressure");
        effect.network.latencySensitivity = numberAt(*network, "latency_sensitivity");
        effect.network.trafficBurstiness = numberAt(*network, "traffic_burstiness");
    }
    if (const Json* database = effects.find("database"); database != nullptr && database->isObject()) {
        effect.database.readPressure = numberAt(*database, "read_pressure");
        effect.database.writePressure = numberAt(*database, "write_pressure");
        effect.database.contention = numberAt(*database, "contention");
        effect.database.replicationLag = numberAt(*database, "replication_lag");
    }
    if (const Json* runtime = effects.find("runtime"); runtime != nullptr && runtime->isObject()) {
        effect.runtime.cpuPressure = numberAt(*runtime, "cpu_pressure");
        effect.runtime.memoryPressure = numberAt(*runtime, "memory_pressure");
        effect.runtime.allocationOrGcPressure = numberAt(*runtime, "allocation_or_gc_pressure");
        effect.runtime.schedulingPressure = numberAt(*runtime, "scheduling_pressure");
    }
    return effect;
}

inline PressureState parsePressureStateAt(const Json& object, const std::string& key)
{
    const Json* effects = object.find(key);
    return effects != nullptr ? parsePressureStateObject(*effects) : PressureState{};
}

inline PressureState parsePressureEffect(const Json& object)
{
    return parsePressureStateAt(object, "pressure_effects");
}

inline std::vector<PressureContextSignal> parsePressureSignals(const Json& object)
{
    std::vector<PressureContextSignal> signals;
    const Json* array = object.find("pressure_signals");
    if (array == nullptr || !array->isArray()) {
        return signals;
    }
    for (const auto& entry : array->asArray()) {
        if (!entry.isObject()) {
            continue;
        }
        signals.push_back({
            .domain = pressureDomainFromId(stringAt(entry, "domain")),
            .name = stringAt(entry, "name", stringAt(entry, "display_name")),
            .summary = stringAt(entry, "summary"),
            .temporary = boolAt(entry, "temporary"),
        });
    }
    return signals;
}

inline void parseCapacityBonusRanges(const Json& object, WorldActionDefinition& action)
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

inline EventDefinition parseEvent(const Json& object)
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
        event.effect.pressureEffect = parsePressureEffect(*effect);
        event.effect.pressureSignals = parsePressureSignals(*effect);
        for (auto& signal : event.effect.pressureSignals) {
            signal.temporary = true;
        }
    }
    event.durationSecondsRange = rangeAtAny(object, "duration_turns", "duration_seconds", event.durationSeconds);
    event.durationSeconds = event.durationSecondsRange.min;
    event.durationTurns = static_cast<int>(numberAt(object, "duration_turns", event.durationTurns > 0 ? static_cast<double>(event.durationTurns) : event.durationSecondsRange.min));
    event.intensityRange = rangeAt(object, "intensity", event.intensity);
    event.intensity = event.intensityRange.min;
    event.repeatable = boolAt(object, "repeatable");
    return event;
}

inline ScenarioObjective parseObjective(const Json& object)
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

inline ScenarioObjective objectiveFromScenarioEntry(const Json& entry, const std::unordered_map<std::string, ScenarioObjective>& objectives, ContentLoadResult& result, const std::string& scenarioId)
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

inline TrafficProfile parseTraffic(const Json& object)
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

inline ScenarioModifierDefinition parseModifier(const Json& object, const std::unordered_map<std::string, EventDefinition>& events)
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
    modifier.pressureContext = parsePressureStateAt(object, "pressure_context");
    modifier.pressureSignals = parsePressureSignals(object);
    for (const auto& eventId : stringsAt(object, "events")) {
        if (const auto it = events.find(eventId); it != events.end()) {
            modifier.events.push_back(it->second);
        }
    }
    return modifier;
}

inline std::vector<NodeScenario> parseNodes(const Json& topology)
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

inline std::vector<LinkScenario> parseLinks(const Json& topology, const std::vector<NodeScenario>& nodes)
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

inline void applyScenarioOverrides(ScenarioDefinition& scenario, const Json& object)
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
    scenario.pressureContext = parsePressureStateAt(object, "pressure_context");
    scenario.pressureSignals = parsePressureSignals(object);
    scenario.requestTimeoutSeconds = numberAt(object, "request_timeout_seconds", scenario.requestTimeoutSeconds);
}

inline std::vector<ScenarioPhase> parsePhases(const Json& object)
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

inline InterventionDefinition parseIntervention(const Json& object)
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

inline WorldActionDefinition parseWorldAction(const Json& object)
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

} // namespace content::loading
