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

const SandboxControls& ScenarioManager::sandboxControls() const
{
    return sandboxControls_;
}

void ScenarioManager::setSandboxTrafficMultiplier(double multiplier)
{
    sandboxControls_.trafficMultiplier = std::clamp(multiplier, 0.1, 8.0);
}

void ScenarioManager::setSandboxLatencyMultiplier(double multiplier)
{
    sandboxControls_.latencyMultiplier = std::clamp(multiplier, 0.1, 8.0);
}

void ScenarioManager::setSandboxQueueBuildup(bool enabled)
{
    sandboxControls_.queueBuildup = enabled;
}

void ScenarioManager::setSandboxSeed(std::uint32_t seed)
{
    sandboxControls_.seed = seed;
    createRun(seed);
}

void ScenarioManager::injectSandboxEvent(const std::string& id, Simulation& simulation)
{
    const std::string contentId = id.rfind("sandbox_", 0) == 0 ? id : "sandbox_" + id;
    const auto preset = std::find_if(run_.activeDefinition.sandboxEvents.begin(), run_.activeDefinition.sandboxEvents.end(), [&](const EventDefinition& event) {
        return event.id == contentId || event.id == id;
    });
    if (preset != run_.activeDefinition.sandboxEvents.end()) {
        EventDefinition event = *preset;
        event.trigger = {.type = EventTriggerType::TimeBased, .timeSeconds = run_.elapsedSeconds};
        eventManager_.inject(std::move(event), run_.elapsedSeconds, run_.turnNumber, currentTransitionDuration().simulationSeconds, simulation);
        return;
    }

    EventDefinition event;
    event.id = "sandbox_" + id;
    event.displayName = id;
    event.name = id;
    event.category = EventCategory::EducationalEvent;
    event.trigger = {.type = EventTriggerType::TimeBased, .timeSeconds = run_.elapsedSeconds};
    event.durationSeconds = 18.0;
    if (id == "traffic_spike" || id == "regional_traffic_spike") {
        event.displayName = id == "regional_traffic_spike" ? "Regional traffic spike" : "Traffic spike";
        event.name = event.displayName;
        event.category = EventCategory::TrafficEvent;
        event.effect = {.type = EventEffectType::TrafficSpike, .trafficMultiplier = id == "regional_traffic_spike" ? 1.7 : 1.45, .burstMultiplier = 1.35};
    } else if (id == "retry_storm") {
        event.displayName = "Retry storm";
        event.name = event.displayName;
        event.category = EventCategory::ReliabilityEvent;
        event.effect = {.type = EventEffectType::RetryStorm, .trafficMultiplier = 1.22, .retryDelayMultiplier = 0.65};
    } else if (id == "db_slowdown") {
        event.displayName = "DB slowdown";
        event.name = event.displayName;
        event.category = EventCategory::InfrastructureEvent;
        event.effect = {.type = EventEffectType::DatabaseSlowdown, .databaseCapacityMultiplier = 0.55};
    } else if (id == "recovery") {
        event.displayName = "Recovery wave";
        event.name = event.displayName;
        event.category = EventCategory::RecoveryEvent;
        event.effect = {.type = EventEffectType::PartialRecovery, .trafficMultiplier = 0.75, .databaseCapacityMultiplier = 1.2, .retryDelayMultiplier = 1.1};
    }
    eventManager_.inject(std::move(event), run_.elapsedSeconds, run_.turnNumber, currentTransitionDuration().simulationSeconds, simulation);
}

void ScenarioManager::clearSandboxEvents()
{
    eventManager_.clear();
}

const EventManager& ScenarioManager::eventManager() const
{
    return eventManager_;
}

ScenarioRunState ScenarioManager::state() const
{
    return run_.state;
}

double ScenarioManager::elapsedSeconds() const
{
    return run_.elapsedSeconds;
}

int ScenarioManager::turnNumber() const
{
    return run_.turnNumber;
}

const GameplayDuration& ScenarioManager::currentTransitionDuration() const
{
    if (const ScenarioPhase* phase = currentPhase(); phase != nullptr) {
        return phase->transitionDuration;
    }
    return run_.activeDefinition.turnDuration;
}

std::string ScenarioManager::visibleCalendarLabel(const Simulation& simulation) const
{
    (void)simulation;
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "Turn %d", run_.turnNumber);
    return buffer;
}

std::string ScenarioManager::archetypeSummary() const
{
    return scenarioArchetypeName(run_.activeDefinition.archetype);
}

std::string ScenarioManager::progressionTierSummary() const
{
    return progressionTierName(run_.activeDefinition.minimumTier);
}

std::string ScenarioManager::activeModifiersSummary() const
{
    if (run_.selectedModifiers.empty()) {
        return "None";
    }
    std::ostringstream stream;
    for (std::size_t i = 0; i < run_.selectedModifiers.size(); ++i) {
        if (i > 0) {
            stream << ", ";
        }
        stream << run_.selectedModifiers[i].name;
    }
    return stream.str();
}
