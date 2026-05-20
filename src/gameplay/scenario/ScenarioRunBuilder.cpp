#include "gameplay/scenario/ScenarioManager.hpp"

#include "content/ContentRegistry.hpp"
#include "content/validation/ValueSpec.hpp"

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

ScenarioDefinition ScenarioManager::createActiveDefinition(std::uint32_t seed) const
{
    ScenarioDefinition definition = scenario_;
    definition.trafficProfile.baseMultiplier = content::sampleRange(definition.trafficProfile.baseMultiplierRange, seed, definition.id + ".traffic.base_multiplier");
    definition.trafficProfile.growthPerSecond = content::sampleRange(definition.trafficProfile.growthPerSecondRange, seed, definition.id + ".traffic.growth_per_turn");
    definition.bursts = instantiateBurst(definition.bursts, seed, definition.id + ".bursts");
    for (std::size_t i = 0; i < definition.phases.size(); ++i) {
        auto& phase = definition.phases[i];
        const std::string key = definition.id + ".phases." + std::to_string(i);
        phase.startTimeSeconds = content::sampleRange(phase.startTimeSecondsRange, seed, key + ".start_time_seconds");
        phase.durationSeconds = content::sampleRange(phase.durationSecondsRange, seed, key + ".duration_turns");
        phase.trafficMultiplier = content::sampleRange(phase.trafficMultiplierRange, seed, key + ".traffic_multiplier");
        if (phase.burstOverride) {
            phase.burstOverride = instantiateBurst(*phase.burstOverride, seed, key + ".burst_override");
        }
    }
    for (std::size_t i = 0; i < definition.events.size(); ++i) {
        definition.events[i] = instantiateEvent(definition.events[i], seed, definition.id + ".events." + std::to_string(i));
    }
    for (std::size_t i = 0; i < definition.sandboxEvents.size(); ++i) {
        definition.sandboxEvents[i] = instantiateEvent(definition.sandboxEvents[i], seed, definition.id + ".sandbox_events." + std::to_string(i));
    }
    for (const auto& modifier : run_.selectedModifiers) {
        applyModifier(definition, modifier);
    }
    applyProceduralLocations(definition, seed);
    applyProgressionTierFilters(definition);
    return definition;
}

std::vector<ScenarioModifierDefinition> ScenarioManager::selectModifiers(std::uint32_t seed) const
{
    std::vector<ScenarioModifierDefinition> selected;
    if (scenario_.optionalModifiers.empty()) {
        return selected;
    }

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> roll(0.0, 1.0);
    for (std::size_t i = 0; i < scenario_.optionalModifiers.size(); ++i) {
        const auto modifier = instantiateModifier(
            scenario_.optionalModifiers[i],
            seed,
            scenario_.id + ".modifiers." + std::to_string(i));
        const double chance = std::clamp(modifier.selectionWeight, 0.0, 1.0);
        if (roll(rng) <= chance) {
            selected.push_back(modifier);
        }
    }
    return selected;
}

void ScenarioManager::applyProgressionTierFilters(ScenarioDefinition& definition) const
{
    const auto& tier = ProgressionRegistry::definition(definition.minimumTier);
    auto filter = [&tier](std::vector<MechanicType>& mechanics) {
        if (tier.availableMechanics.empty()) {
            return;
        }
        mechanics.erase(
            std::remove_if(mechanics.begin(), mechanics.end(), [&tier](MechanicType mechanic) {
                return std::find(tier.availableMechanics.begin(), tier.availableMechanics.end(), mechanic) == tier.availableMechanics.end();
            }),
            mechanics.end());
    };
    filter(definition.allowedMechanics);
    filter(definition.startingInterventions);
    filter(definition.unlockableInterventions);
}

void ScenarioManager::initializeScenarioRunState()
{
    run_.activeObjectiveIds.clear();
    run_.completedObjectiveIds.clear();
    run_.unlockedInterventions = run_.activeDefinition.startingInterventions.empty()
        ? run_.activeDefinition.allowedMechanics
        : run_.activeDefinition.startingInterventions;
    if (run_.activeDefinition.sandboxLab) {
        run_.unlockedInterventions = run_.activeDefinition.allowedMechanics;
    }
    for (const auto mechanic : run_.activeDefinition.disabledInterventions) {
        run_.unlockedInterventions.erase(
            std::remove(run_.unlockedInterventions.begin(), run_.unlockedInterventions.end(), mechanic),
            run_.unlockedInterventions.end());
    }
    run_.unlockedScenarioIds.clear();
    run_.unlockedMetrics.clear();
    run_.unlockedOverlays.clear();
    run_.feedbackMessages.clear();

    for (const auto& objective : run_.activeDefinition.objectives) {
        if (objective.startsActive || run_.activeObjectiveIds.empty()) {
            run_.activeObjectiveIds.push_back(objective.id);
        }
    }
}

void ScenarioManager::applyModifier(ScenarioDefinition& definition, const ScenarioModifierDefinition& modifier) const
{
    definition.trafficProfile.baseMultiplier *= modifier.trafficMultiplier;
    if (modifier.databaseHeavyShare) {
        definition.requestTypes.lightweightShare = std::clamp(1.0 - *modifier.databaseHeavyShare, 0.0, 1.0);
    }
    if (modifier.burstOverride) {
        definition.bursts = *modifier.burstOverride;
    }
    definition.events.insert(definition.events.end(), modifier.events.begin(), modifier.events.end());
}

