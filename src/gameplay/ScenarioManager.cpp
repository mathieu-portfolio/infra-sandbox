#include "gameplay/ScenarioManager.hpp"

#include <algorithm>
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
    if (id == "add_cache") return MechanicType::AddCache;
    if (id == "add_queue") return MechanicType::AddQueue;
    if (id == "add_read_replica") return MechanicType::AddReadReplica;
    if (id == "add_regional_cache") return MechanicType::AddRegionalCache;
    if (id == "toggle_retries") return MechanicType::ToggleRetries;
    if (id == "clear_cache") return MechanicType::ClearCache;
    if (id == "throttle_traffic") return MechanicType::ThrottleTraffic;
    return MechanicType::ScaleUp;
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
    run_.objectiveProgress = 0.0;
    run_.currentPhaseIndex = run_.activeDefinition.phases.empty() ? -1 : 0;
    initializeScenarioRunState();
    actionsTriggered_.clear();
    eventManager_.reset(run_.activeDefinition.events);
}

void ScenarioManager::update(double dt, Simulation& simulation)
{
    run_.elapsedSeconds += dt;
    run_.currentPhaseIndex = -1;
    for (int i = 0; i < static_cast<int>(run_.activeDefinition.phases.size()); ++i) {
        const auto& phase = run_.activeDefinition.phases[i];
        if (run_.elapsedSeconds >= phase.startTimeSeconds && run_.elapsedSeconds < phase.startTimeSeconds + phase.durationSeconds) {
            run_.currentPhaseIndex = i;
        }
    }

    eventManager_.update(dt, run_.elapsedSeconds, run_.currentPhaseIndex, simulation);
    const double phaseElapsed = currentPhase() != nullptr
        ? run_.elapsedSeconds - currentPhase()->startTimeSeconds
        : run_.elapsedSeconds;
    simulation.setScenarioTime(run_.elapsedSeconds, phaseElapsed);
    applyPhaseToSimulation(simulation);
    updateState(simulation);
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

void ScenarioManager::injectSandboxEvent(const std::string& id)
{
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
    eventManager_.inject(std::move(event), run_.elapsedSeconds);
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
    (void)seed;
    for (const auto& modifier : run_.selectedModifiers) {
        applyModifier(definition, modifier);
    }
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
    for (const auto& modifier : scenario_.optionalModifiers) {
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
        multiplier += run_.elapsedSeconds * scenario.trafficProfile.growthPerSecond;
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
    simulation.setScenarioLatencyMultiplier(run_.activeDefinition.sandboxLab ? sandboxControls_.latencyMultiplier : 1.0);
    simulation.setScenarioDatabaseHeavyShareOverride(eventModifiers.databaseHeavyShare);
    simulation.setScenarioRetryDelayMultiplier(eventModifiers.retryDelayMultiplier);

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
            run_.objectiveProgress = objective.durationSeconds > 0.0 ? std::min(1.0, run_.elapsedSeconds / objective.durationSeconds) : 0.0;
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
        if (reward.id == "scale_up") unlockIntervention(MechanicType::ScaleUp);
        else if (reward.id == "add_cache") unlockIntervention(MechanicType::AddCache);
        else if (reward.id == "add_queue") unlockIntervention(MechanicType::AddQueue);
        else if (reward.id == "add_read_replica") unlockIntervention(MechanicType::AddReadReplica);
        else if (reward.id == "add_regional_cache") unlockIntervention(MechanicType::AddRegionalCache);
        else if (reward.id == "toggle_retries") unlockIntervention(MechanicType::ToggleRetries);
        else if (reward.id == "clear_cache") unlockIntervention(MechanicType::ClearCache);
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
