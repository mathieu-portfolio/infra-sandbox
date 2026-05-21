#include "content/ContentRegistry.hpp"

#include "content/loading/ContentJsonAccess.hpp"
#include "content/loading/ContentEnumParsers.hpp"
#include "content/loading/ContentDomainParsers.hpp"
#include "content/loading/ContentLayerLoader.hpp"
#include "core/parsing/Json.hpp"
#include "core/validation/ValueSpec.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_map>

namespace content {
namespace {
using namespace content::loading;

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
