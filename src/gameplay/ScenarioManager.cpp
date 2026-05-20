#include "gameplay/ScenarioManager.hpp"

#include "content/ContentRegistry.hpp"
#include "content/ValueSpec.hpp"

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

std::optional<EventLogEntry> ScenarioManager::rollPlanningEvent(const Simulation& simulation)
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

void ScenarioManager::notifyActionTriggered(MechanicType mechanic)
{
    actionsTriggered_.push_back(mechanic);
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

void ScenarioManager::injectSandboxEvent(const std::string& id, const Simulation& simulation)
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

void ScenarioManager::applyPhaseToSimulation(Simulation& simulation) const
{
    const auto& scenario = run_.activeDefinition;
    double multiplier = scenario.trafficProfile.baseMultiplier;
    if (scenario.trafficProfile.type == TrafficProfileType::GradualGrowth) {
        multiplier += static_cast<double>(run_.turnNumber) * scenario.trafficProfile.growthPerSecond;
    }

    if (const ScenarioPhase* phase = currentPhase()) {
        multiplier *= phase->trafficMultiplier;
        if (phase->burstOverride) {
            simulation.setScenarioBurst(*phase->burstOverride);
        } else {
            simulation.clearScenarioBurstOverride();
        }
    } else {
        simulation.clearScenarioBurstOverride();
    }

    const auto eventModifiers = eventManager_.modifiers();
    multiplier *= eventModifiers.trafficMultiplier;
    if (run_.activeDefinition.sandboxLab) {
        multiplier *= sandboxControls_.trafficMultiplier;
        if (sandboxControls_.queueBuildup) {
            multiplier *= 2.2;
        }
    }
    if (eventModifiers.burstMultiplier != 1.0) {
        auto burst = scenario.bursts;
        if (const auto* phase = currentPhase(); phase != nullptr && phase->burstOverride) {
            burst = *phase->burstOverride;
        }
        burst.enabled = true;
        burst.multiplier *= eventModifiers.burstMultiplier;
        simulation.setScenarioBurst(burst);
    }
    simulation.setScenarioDatabaseCapacityMultiplier(eventModifiers.databaseCapacityMultiplier);
    simulation.setScenarioLatencyMultiplier(eventModifiers.latencyMultiplier * (run_.activeDefinition.sandboxLab ? sandboxControls_.latencyMultiplier : 1.0));
    simulation.setScenarioDatabaseHeavyShareOverride(eventModifiers.databaseHeavyShare);
    simulation.setScenarioRetryDelayMultiplier(eventModifiers.retryDelayMultiplier);
    simulation.setLocalizedEventModifiers(eventManager_.localizedModifiers());

    auto allowedMechanics = run_.unlockedInterventions;
    allowedMechanics.insert(
        allowedMechanics.end(),
        eventModifiers.unlockedMechanics.begin(),
        eventModifiers.unlockedMechanics.end());
    simulation.setScenarioTrafficMultiplier(multiplier);
    simulation.setAllowedMechanics(allowedMechanics);
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
