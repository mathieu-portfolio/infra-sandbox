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

ScenarioManager::ScenarioManager(ScenarioDefinition scenario)
    : scenario_(std::move(scenario))
{
    reset();
}

void ScenarioManager::load(ScenarioDefinition scenario)
{
    scenario_ = std::move(scenario);
    reset();
}

void ScenarioManager::reset()
{
    createRun(nextSeed_);
}

void ScenarioManager::createRun(std::uint32_t seed)
{
    if (progressionState_.unlockedScenarios.empty()) {
        const auto scenarios = ScenarioRegistry::createAll();
        const auto firstPlayable = std::find_if(scenarios.begin(), scenarios.end(), [](const ScenarioDefinition& scenario) {
            return !scenario.sandboxLab;
        });
        if (firstPlayable != scenarios.end()) {
            progressionState_.unlockedScenarios.insert(firstPlayable->id);
        }
    }
    progressionState_.unlockedScenarios.insert(scenario_.id);
    run_.seed = seed;
    run_.selectedModifiers = selectModifiers(seed);
    run_.activeDefinition = createActiveDefinition(seed);
    run_.state = ScenarioRunState::Running;
    run_.elapsedSeconds = 0.0;
    run_.turnNumber = 0;
    run_.turnElapsedSeconds = 0.0;
    run_.calendarElapsedDays = 0.0;
    run_.objectiveProgress = 0.0;
    run_.currentPhaseIndex = run_.activeDefinition.phases.empty() ? -1 : 0;
    initializeScenarioRunState();
    actionsTriggered_.clear();
    eventManager_.reset(run_.activeDefinition.events, seed);
}

void ScenarioManager::beginTurn()
{
    ++run_.turnNumber;
    run_.turnElapsedSeconds = 0.0;
}

void ScenarioManager::update(double dt, Simulation& simulation)
{
    if (run_.turnNumber == 0) {
        beginTurn();
    }
    run_.elapsedSeconds += dt;
    run_.turnElapsedSeconds += dt;
    run_.calendarElapsedDays += dt * calendarDaysPerSimulationSecond_;
    run_.currentPhaseIndex = -1;
    for (int i = 0; i < static_cast<int>(run_.activeDefinition.phases.size()); ++i) {
        const auto& phase = run_.activeDefinition.phases[i];
        const bool turnPhase = phase.startTurn > 0 || phase.durationTurns > 0;
        if (turnPhase) {
            const int start = std::max(1, phase.startTurn);
            const int end = phase.durationTurns > 0 ? start + phase.durationTurns : start + 1;
            if (run_.turnNumber >= start && run_.turnNumber < end) {
                run_.currentPhaseIndex = i;
            }
        } else if (run_.elapsedSeconds >= phase.startTimeSeconds && run_.elapsedSeconds < phase.startTimeSeconds + phase.durationSeconds) {
            run_.currentPhaseIndex = i;
        }
    }

    eventManager_.update(dt, run_.elapsedSeconds, run_.turnNumber, currentTransitionDuration().simulationSeconds, run_.currentPhaseIndex, simulation);
    const double phaseElapsed = currentPhase() != nullptr
        ? run_.elapsedSeconds - currentPhase()->startTimeSeconds
        : run_.elapsedSeconds;
    simulation.setScenarioTime(run_.elapsedSeconds, phaseElapsed, run_.calendarElapsedDays);
    applyPhaseToSimulation(simulation);
    updateState(simulation);
}

std::optional<EventLogEntry> ScenarioManager::rollPlanningEvent(Simulation& simulation)
{
    return eventManager_.rollPlanningEvent(run_.elapsedSeconds, run_.turnNumber, currentTransitionDuration().simulationSeconds, run_.currentPhaseIndex, simulation);
}

std::size_t ScenarioManager::eventLogSize() const
{
    return eventManager_.recentEventCount();
}

std::vector<EventLogEntry> ScenarioManager::eventsSince(std::size_t startIndex) const
{
    return eventManager_.eventsSince(startIndex);
}

void ScenarioManager::setCalendarProgressionScale(double calendarDaysPerSimulationSecond)
{
    calendarDaysPerSimulationSecond_ = std::max(0.0, calendarDaysPerSimulationSecond);
}

const ScenarioDefinition& ScenarioManager::definition() const
{
    return run_.activeDefinition;
}

const ScenarioDefinition& ScenarioManager::staticDefinition() const
{
    return scenario_;
}

const ScenarioRun& ScenarioManager::run() const
{
    return run_;
}

const ScenarioPhase* ScenarioManager::currentPhase() const
{
    if (run_.currentPhaseIndex < 0 || run_.currentPhaseIndex >= static_cast<int>(run_.activeDefinition.phases.size())) {
        return nullptr;
    }
    return &run_.activeDefinition.phases[run_.currentPhaseIndex];
}

std::string ScenarioManager::objectiveSummary() const
{
    const auto it = std::find_if(run_.activeDefinition.objectives.begin(), run_.activeDefinition.objectives.end(), [this](const ScenarioObjective& objective) {
        return isObjectiveActive(objective);
    });
    if (it == run_.activeDefinition.objectives.end()) {
        return "Observe system behavior";
    }
    return it->summary;
}

std::string ScenarioManager::focusSummary() const
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < run_.activeDefinition.educationalFocus.size(); ++i) {
        if (i > 0) {
            stream << ", ";
        }
        stream << focusName(run_.activeDefinition.educationalFocus[i]);
    }
    return stream.str();
}

std::string ScenarioManager::availableMechanicsSummary() const
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < run_.activeDefinition.allowedMechanics.size(); ++i) {
        if (i > 0) {
            stream << ", ";
        }
        stream << MechanicRegistry::definition(run_.activeDefinition.allowedMechanics[i]).displayName;
    }
    return stream.str();
}

const ProgressionState& ScenarioManager::progressionState() const
{
    return progressionState_;
}

bool ScenarioManager::isScenarioUnlocked(const ScenarioDefinition& scenario) const
{
    if (scenario.sandboxLab) {
        return true;
    }
    if (scenario.id == scenario_.id || progressionState_.unlockedScenarios.contains(scenario.id)) {
        return true;
    }
    return scenario.requiredCompletedScenarios.empty()
        && !scenario.sandboxLab
        && scenario.id == Scenario::createDefault().id;
}

