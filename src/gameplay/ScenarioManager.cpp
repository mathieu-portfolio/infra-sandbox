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
    run_.seed = seed;
    run_.selectedModifiers = selectModifiers(seed);
    run_.activeDefinition = createActiveDefinition(seed);
    run_.state = ScenarioRunState::Running;
    run_.elapsedSeconds = 0.0;
    run_.objectiveProgress = 0.0;
    run_.currentPhaseIndex = run_.activeDefinition.phases.empty() ? -1 : 0;
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
    if (run_.activeDefinition.objectives.empty()) {
        return "Observe system behavior";
    }
    return run_.activeDefinition.objectives.front().summary;
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
    if (!tier.availableMechanics.empty()) {
        definition.allowedMechanics.erase(
            std::remove_if(definition.allowedMechanics.begin(), definition.allowedMechanics.end(), [&tier](MechanicType mechanic) {
                return std::find(tier.availableMechanics.begin(), tier.availableMechanics.end(), mechanic) == tier.availableMechanics.end();
            }),
            definition.allowedMechanics.end());
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
    simulation.setScenarioDatabaseHeavyShareOverride(eventModifiers.databaseHeavyShare);
    simulation.setScenarioRetryDelayMultiplier(eventModifiers.retryDelayMultiplier);

    auto allowedMechanics = scenario.allowedMechanics;
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
        if (objective.type == ScenarioObjectiveType::SurviveDuration) {
            run_.objectiveProgress = objective.durationSeconds > 0.0 ? std::min(1.0, run_.elapsedSeconds / objective.durationSeconds) : 0.0;
            if (run_.elapsedSeconds >= objective.durationSeconds) {
                run_.state = ScenarioRunState::Succeeded;
            }
        }
    }
}
