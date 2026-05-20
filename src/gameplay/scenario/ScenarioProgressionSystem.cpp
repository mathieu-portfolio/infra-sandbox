#include "gameplay/scenario/ScenarioManager.hpp"

#include "content/ContentRegistry.hpp"
#include "core/validation/ValueSpec.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <random>
#include <sstream>
#include <utility>

namespace {
const char* focusName(EducationalFocus focus)
{
    switch (focus) {
    case EducationalFocus::Queues:
        return "Queues";
    case EducationalFocus::Caching:
        return "Caching";
    case EducationalFocus::Scaling:
        return "Scaling";
    case EducationalFocus::Reliability:
        return "Reliability";
    case EducationalFocus::Persistence:
        return "Persistence";
    case EducationalFocus::Latency:
        return "Latency";
    case EducationalFocus::Geography:
        return "Geography";
    }
    return "Unknown";
}

MechanicType mechanicFromObjectiveId(const std::string& id)
{
    const auto& interventions = content::ContentRegistry::instance().interventions();
    const auto intervention = std::find_if(interventions.begin(), interventions.end(), [&id](const content::InterventionDefinition& definition) {
        return definition.id == id;
    });
    if (intervention != interventions.end()) {
        return intervention->mechanic;
    }
    if (id == "add_cache") return MechanicType::AddCache;
    if (id == "add_queue") return MechanicType::AddQueue;
    if (id == "add_read_replica") return MechanicType::AddReadReplica;
    if (id == "add_regional_cache") return MechanicType::AddRegionalCache;
    if (id == "toggle_retries") return MechanicType::ToggleRetries;
    if (id == "clear_cache") return MechanicType::ClearCache;
    if (id == "throttle_traffic") return MechanicType::ThrottleTraffic;
    return MechanicType::ScaleUp;
}

double scaledMultiplier(double multiplier, double intensity)
{
    return 1.0 + (multiplier - 1.0) * intensity;
}

BurstScenario instantiateBurst(BurstScenario burst, std::uint32_t seed, const std::string& key)
{
    burst.multiplier = content::sampleRange(burst.multiplierRange, seed, key + ".multiplier");
    burst.periodSeconds = content::sampleRange(burst.periodSecondsRange, seed, key + ".period_turns");
    burst.durationSeconds = content::sampleRange(burst.durationSecondsRange, seed, key + ".duration_turns");
    return burst;
}

EventDefinition instantiateEvent(EventDefinition event, std::uint32_t seed, const std::string& key)
{
    event.trigger.timeSeconds = content::sampleRange(event.trigger.timeSecondsRange, seed, key + ".trigger.time_seconds");
    event.trigger.delaySeconds = content::sampleRange(event.trigger.delaySecondsRange, seed, key + ".trigger.delay_seconds");
    event.effect.trafficMultiplier = content::sampleRange(event.effect.trafficMultiplierRange, seed, key + ".effect.traffic_multiplier");
    event.effect.burstMultiplier = content::sampleRange(event.effect.burstMultiplierRange, seed, key + ".effect.burst_multiplier");
    event.effect.databaseCapacityMultiplier = content::sampleRange(event.effect.databaseCapacityMultiplierRange, seed, key + ".effect.database_capacity_multiplier");
    event.effect.latencyMultiplier = content::sampleRange(event.effect.latencyMultiplierRange, seed, key + ".effect.latency_multiplier");
    event.effect.retryDelayMultiplier = content::sampleRange(event.effect.retryDelayMultiplierRange, seed, key + ".effect.retry_delay_multiplier");
    event.effect.regionalDemandRatePerSecond = content::sampleRange(event.effect.regionalDemandRatePerSecondRange, seed, key + ".effect.regional_demand_rate_per_second");
    if (event.effect.databaseHeavyShareRange) {
        event.effect.databaseHeavyShare = content::sampleRange(*event.effect.databaseHeavyShareRange, seed, key + ".effect.database_heavy_share");
    }
    event.durationSeconds = content::sampleRange(event.durationSecondsRange, seed, key + ".duration_turns");
    if (event.durationTurns > 0 && event.durationSecondsRange.min == 10.0 && event.durationSecondsRange.max == 10.0) {
        event.durationSeconds = 0.0;
    }
    event.intensity = content::sampleRange(event.intensityRange, seed, key + ".intensity");
    event.effect.trafficMultiplier = scaledMultiplier(event.effect.trafficMultiplier, event.intensity);
    event.effect.burstMultiplier = scaledMultiplier(event.effect.burstMultiplier, event.intensity);
    event.effect.databaseCapacityMultiplier = scaledMultiplier(event.effect.databaseCapacityMultiplier, event.intensity);
    event.effect.latencyMultiplier = scaledMultiplier(event.effect.latencyMultiplier, event.intensity);
    event.effect.retryDelayMultiplier = scaledMultiplier(event.effect.retryDelayMultiplier, event.intensity);
    return event;
}


std::string regionDisplayName(const std::string& region)
{
    if (region == "NorthAmerica") return "North America";
    if (region == "AsiaPacific") return "Asia Pacific";
    if (region == "SouthAmerica") return "South America";
    return region.empty() ? "Regional" : region;
}

std::string roleDisplayName(const NodeScenario& node)
{
    switch (node.type) {
    case NodeType::ClientCluster:
        return "users";
    case NodeType::ApiService:
        return node.id.find("secondary") != std::string::npos ? "API secondary" : "API cluster";
    case NodeType::Database:
        return "database";
    case NodeType::Cache:
        return "cache";
    case NodeType::QueueBroker:
        return "queue";
    case NodeType::Worker:
        return "worker";
    case NodeType::LoadBalancer:
        return "load balancer";
    default:
        break;
    }
    return "node";
}

const RegionDefinition* findRegionDefinition(const std::string& region)
{
    const auto& definitions = GeographicRegistry::definitions();
    const auto it = std::find_if(definitions.begin(), definitions.end(), [&region](const RegionDefinition& definition) {
        return std::string(definition.name) == region;
    });
    return it != definitions.end() ? &(*it) : nullptr;
}

GeoLocation chooseProceduralBaseLocation(
    const ScenarioDefinition& definition,
    const NodeScenario& node,
    std::uint32_t seed,
    const std::string& key)
{
    std::vector<std::string> candidateRegions;
    if (const auto it = definition.proceduralLocationRegions.find(node.id); it != definition.proceduralLocationRegions.end()) {
        candidateRegions = it->second;
    } else if (node.geoLocation && !node.geoLocation->regionName.empty()) {
        candidateRegions.push_back(node.geoLocation->regionName);
    }

    if (candidateRegions.empty()) {
        for (const auto& definition : GeographicRegistry::definitions()) {
            candidateRegions.emplace_back(definition.name);
        }
    }

    const int index = std::clamp(
        static_cast<int>(content::sampleNumber(seed, key + ".region", 0.0, static_cast<double>(candidateRegions.size()))),
        0,
        static_cast<int>(candidateRegions.size()) - 1);
    if (const RegionDefinition* region = findRegionDefinition(candidateRegions[static_cast<std::size_t>(index)])) {
        return region->center;
    }
    return node.geoLocation.value_or(GeoLocation{});
}

void applyProceduralLocations(ScenarioDefinition& definition, std::uint32_t seed)
{
    if (!definition.proceduralLocations) {
        return;
    }

    const double jitter = std::max(0.0, definition.proceduralLocationJitterDegrees);
    for (auto& node : definition.nodes) {
        const std::string key = definition.id + ".locations." + node.id;
        GeoLocation location = chooseProceduralBaseLocation(definition, node, seed, key);
        location.latitude = std::clamp(
            location.latitude + content::sampleNumber(seed, key + ".lat", -jitter, jitter),
            -70.0,
            70.0);
        location.longitude = std::clamp(
            location.longitude + content::sampleNumber(seed, key + ".lon", -jitter * 1.8, jitter * 1.8),
            -175.0,
            175.0);
        node.geoLocation = location;
        node.name = regionDisplayName(location.regionName) + " " + roleDisplayName(node);
        node.position = MapProjection::projectEquirectangular(location);
    }
}

ScenarioModifierDefinition instantiateModifier(ScenarioModifierDefinition modifier, std::uint32_t seed, const std::string& key)
{
    modifier.selectionWeight = content::sampleRange(modifier.selectionWeightRange, seed, key + ".selection_weight");
    modifier.trafficMultiplier = content::sampleRange(modifier.trafficMultiplierRange, seed, key + ".traffic_multiplier");
    if (modifier.databaseHeavyShareRange) {
        modifier.databaseHeavyShare = content::sampleRange(*modifier.databaseHeavyShareRange, seed, key + ".database_heavy_share");
    }
    if (modifier.burstOverride) {
        modifier.burstOverride = instantiateBurst(*modifier.burstOverride, seed, key + ".burst_override");
    }
    for (std::size_t i = 0; i < modifier.events.size(); ++i) {
        modifier.events[i] = instantiateEvent(modifier.events[i], seed, key + ".events." + std::to_string(i));
    }
    return modifier;
}
}

void ScenarioManager::applyEngineeringCapacityBonus(const EngineeringCapacity& bonus)
{
    auto& capacity = run_.activeDefinition.engineeringCapacity;
    capacity = applyEngineeringCapacityBudgetCap(capacity, bonus);
}

void ScenarioManager::notifyActionTriggered(MechanicType mechanic)
{
    actionsTriggered_.push_back(mechanic);
}

void ScenarioManager::updateState(const Simulation& simulation)
{
    run_.state = ScenarioRunState::Running;
    for (const auto& failure : run_.activeDefinition.failureConditions) {
        if (failure.type == ScenarioObjectiveType::MaxLatency && simulation.metrics().averageLatencySeconds > failure.threshold) {
            run_.state = ScenarioRunState::RecoverableFailure;
        }
        if (failure.type == ScenarioObjectiveType::MaxErrorRate && simulation.metrics().timeoutRatePerSecond > failure.threshold) {
            run_.state = ScenarioRunState::RecoverableFailure;
        }
    }

    for (const auto& objective : run_.activeDefinition.objectives) {
        if (!isObjectiveActive(objective) || isObjectiveCompleted(objective)) {
            continue;
        }
        if (objective.conditionType == ObjectiveConditionType::SurviveDuration) {
            if (objective.durationTurns > 0) {
                run_.objectiveProgress = std::min(1.0, static_cast<double>(run_.turnNumber) / static_cast<double>(objective.durationTurns));
            } else {
                run_.objectiveProgress = objective.durationSeconds > 0.0 ? std::min(1.0, run_.elapsedSeconds / objective.durationSeconds) : 0.0;
            }
        }
        if (objectiveSatisfied(objective, simulation)) {
            completeObjective(objective);
        }
    }
}

bool ScenarioManager::isObjectiveActive(const ScenarioObjective& objective) const
{
    return std::find(run_.activeObjectiveIds.begin(), run_.activeObjectiveIds.end(), objective.id) != run_.activeObjectiveIds.end();
}

bool ScenarioManager::isObjectiveCompleted(const ScenarioObjective& objective) const
{
    return std::find(run_.completedObjectiveIds.begin(), run_.completedObjectiveIds.end(), objective.id) != run_.completedObjectiveIds.end();
}

bool ScenarioManager::objectiveSatisfied(const ScenarioObjective& objective, const Simulation& simulation) const
{
    const auto& metrics = simulation.metrics();
    const auto& pressure = simulation.pressure();
    switch (objective.conditionType) {
    case ObjectiveConditionType::SurviveDuration:
        if (objective.durationTurns > 0) {
            return run_.turnNumber >= objective.durationTurns;
        }
        return objective.durationSeconds > 0.0 && run_.elapsedSeconds >= objective.durationSeconds;
    case ObjectiveConditionType::PressureDetected:
        return pressure.dominantPressure == objective.pressure;
    case ObjectiveConditionType::PressureBelow:
        if (objective.pressure == PressureCategory::QueuePressure) {
            return metrics.apiQueueDepth + metrics.databaseQueueDepth <= static_cast<int>(objective.threshold);
        }
        if (objective.pressure == PressureCategory::PersistencePressure) {
            const auto nodePressure = std::find_if(pressure.nodes.begin(), pressure.nodes.end(), [&pressure](const NodePressure& entry) {
                return entry.nodeId == pressure.topOverloadedNodeId;
            });
            return pressure.dominantPressure != PressureCategory::PersistencePressure
                || (nodePressure != pressure.nodes.end() && nodePressure->queuePressure <= objective.threshold);
        }
        if (objective.pressure == PressureCategory::FailurePressure) {
            return metrics.timeoutRatePerSecond <= objective.threshold;
        }
        return pressure.dominantPressure != objective.pressure;
    case ObjectiveConditionType::MetricBelow:
        if (objective.conditionMetric == "latency") return metrics.averageLatencySeconds <= objective.threshold;
        if (objective.conditionMetric == "timeout_rate") return metrics.timeoutRatePerSecond <= objective.threshold;
        if (objective.conditionMetric == "api_queue") return metrics.apiQueueDepth <= static_cast<int>(objective.threshold);
        if (objective.conditionMetric == "database_queue") return metrics.databaseQueueDepth <= static_cast<int>(objective.threshold);
        if (objective.conditionMetric == "retry_pressure") {
            double maxRetry = 0.0;
            for (const auto& nodePressure : pressure.nodes) maxRetry = std::max(maxRetry, nodePressure.retryContribution);
            return maxRetry <= objective.threshold;
        }
        if (objective.conditionMetric == "dependency_pressure") {
            double maxDependency = 0.0;
            for (const auto& nodePressure : pressure.nodes) maxDependency = std::max(maxDependency, nodePressure.dependencyPressure);
            return maxDependency <= objective.threshold;
        }
        if (objective.conditionMetric == "instability") {
            double maxInstability = 0.0;
            for (const auto& nodePressure : pressure.nodes) maxInstability = std::max(maxInstability, nodePressure.instability);
            return maxInstability <= objective.threshold;
        }
        return false;
    case ObjectiveConditionType::ActionUsed:
        return std::find(actionsTriggered_.begin(), actionsTriggered_.end(), mechanicFromObjectiveId(objective.conditionMetric)) != actionsTriggered_.end();
    }
    return false;
}

void ScenarioManager::completeObjective(const ScenarioObjective& objective)
{
    if (!isObjectiveCompleted(objective)) {
        run_.completedObjectiveIds.push_back(objective.id);
    }
    run_.activeObjectiveIds.erase(
        std::remove(run_.activeObjectiveIds.begin(), run_.activeObjectiveIds.end(), objective.id),
        run_.activeObjectiveIds.end());
    for (const auto& next : objective.nextObjectives) {
        if (std::find(run_.activeObjectiveIds.begin(), run_.activeObjectiveIds.end(), next) == run_.activeObjectiveIds.end()
            && std::find(run_.completedObjectiveIds.begin(), run_.completedObjectiveIds.end(), next) == run_.completedObjectiveIds.end()) {
            run_.activeObjectiveIds.push_back(next);
        }
    }
    for (const auto& reward : objective.rewards) {
        applyReward(reward);
    }
}

void ScenarioManager::applyReward(const ObjectiveReward& reward)
{
    switch (reward.type) {
    case ObjectiveRewardType::UnlockIntervention:
        unlockIntervention(mechanicFromObjectiveId(reward.id));
        break;
    case ObjectiveRewardType::UnlockMetric:
        run_.unlockedMetrics.push_back(reward.id);
        progressionState_.unlockedMetrics.insert(reward.id);
        break;
    case ObjectiveRewardType::UnlockOverlay:
        run_.unlockedOverlays.push_back(reward.id);
        progressionState_.unlockedOverlays.insert(reward.id);
        break;
    case ObjectiveRewardType::UnlockScenarioPhase:
        break;
    case ObjectiveRewardType::UnlockScenario:
        run_.unlockedScenarioIds.push_back(reward.id);
        progressionState_.unlockedScenarios.insert(reward.id);
        break;
    case ObjectiveRewardType::EmitFeedback:
        if (!reward.message.empty()) run_.feedbackMessages.push_back(reward.message);
        break;
    case ObjectiveRewardType::CompleteScenario:
        completeScenario();
        break;
    }
    if (!reward.message.empty() && reward.type != ObjectiveRewardType::EmitFeedback) {
        run_.feedbackMessages.push_back(reward.message);
    }
}

void ScenarioManager::unlockIntervention(MechanicType mechanic)
{
    if (std::find(run_.activeDefinition.unlockableInterventions.begin(), run_.activeDefinition.unlockableInterventions.end(), mechanic) == run_.activeDefinition.unlockableInterventions.end()
        && std::find(run_.activeDefinition.allowedMechanics.begin(), run_.activeDefinition.allowedMechanics.end(), mechanic) == run_.activeDefinition.allowedMechanics.end()) {
        return;
    }
    if (std::find(run_.unlockedInterventions.begin(), run_.unlockedInterventions.end(), mechanic) == run_.unlockedInterventions.end()) {
        run_.unlockedInterventions.push_back(mechanic);
    }
}

void ScenarioManager::completeScenario()
{
    run_.state = ScenarioRunState::Succeeded;
    progressionState_.completedScenarios.insert(run_.activeDefinition.id);
    for (const auto& scenarioId : run_.activeDefinition.unlocksScenarios) {
        progressionState_.unlockedScenarios.insert(scenarioId);
        run_.unlockedScenarioIds.push_back(scenarioId);
    }
    for (const auto& tag : run_.activeDefinition.tags) {
        progressionState_.unlockedConcepts.insert(tag);
    }
}
