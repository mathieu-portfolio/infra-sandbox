#include "gameplay/ScenarioManager.hpp"

#include <algorithm>
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
    state_ = ScenarioRunState::Running;
    elapsedSeconds_ = 0.0;
    currentPhaseIndex_ = scenario_.phases.empty() ? -1 : 0;
    eventManager_.reset(scenario_.events);
}

void ScenarioManager::update(double dt, Simulation& simulation)
{
    elapsedSeconds_ += dt;
    currentPhaseIndex_ = -1;
    for (int i = 0; i < static_cast<int>(scenario_.phases.size()); ++i) {
        const auto& phase = scenario_.phases[i];
        if (elapsedSeconds_ >= phase.startTimeSeconds && elapsedSeconds_ < phase.startTimeSeconds + phase.durationSeconds) {
            currentPhaseIndex_ = i;
        }
    }

    eventManager_.update(dt, elapsedSeconds_, currentPhaseIndex_, simulation);
    const double phaseElapsed = currentPhase() != nullptr
        ? elapsedSeconds_ - currentPhase()->startTimeSeconds
        : elapsedSeconds_;
    simulation.setScenarioTime(elapsedSeconds_, phaseElapsed);
    applyPhaseToSimulation(simulation);
    updateState(simulation);
}

const ScenarioDefinition& ScenarioManager::definition() const
{
    return scenario_;
}

const ScenarioPhase* ScenarioManager::currentPhase() const
{
    if (currentPhaseIndex_ < 0 || currentPhaseIndex_ >= static_cast<int>(scenario_.phases.size())) {
        return nullptr;
    }
    return &scenario_.phases[currentPhaseIndex_];
}

std::string ScenarioManager::objectiveSummary() const
{
    if (scenario_.objectives.empty()) {
        return "Observe system behavior";
    }
    return scenario_.objectives.front().summary;
}

std::string ScenarioManager::focusSummary() const
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < scenario_.educationalFocus.size(); ++i) {
        if (i > 0) {
            stream << ", ";
        }
        stream << focusName(scenario_.educationalFocus[i]);
    }
    return stream.str();
}

std::string ScenarioManager::availableMechanicsSummary() const
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < scenario_.allowedMechanics.size(); ++i) {
        if (i > 0) {
            stream << ", ";
        }
        stream << MechanicRegistry::definition(scenario_.allowedMechanics[i]).displayName;
    }
    return stream.str();
}

const EventManager& ScenarioManager::eventManager() const
{
    return eventManager_;
}

ScenarioRunState ScenarioManager::state() const
{
    return state_;
}

double ScenarioManager::elapsedSeconds() const
{
    return elapsedSeconds_;
}

void ScenarioManager::applyPhaseToSimulation(Simulation& simulation) const
{
    double multiplier = scenario_.trafficProfile.baseMultiplier;
    if (scenario_.trafficProfile.type == TrafficProfileType::GradualGrowth) {
        multiplier += elapsedSeconds_ * scenario_.trafficProfile.growthPerSecond;
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
        auto burst = scenario_.bursts;
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

    auto allowedMechanics = scenario_.allowedMechanics;
    allowedMechanics.insert(
        allowedMechanics.end(),
        eventModifiers.unlockedMechanics.begin(),
        eventModifiers.unlockedMechanics.end());
    simulation.setScenarioTrafficMultiplier(multiplier);
    simulation.setAllowedMechanics(allowedMechanics);
}

void ScenarioManager::updateState(const Simulation& simulation)
{
    state_ = ScenarioRunState::Running;
    for (const auto& failure : scenario_.failureConditions) {
        if (failure.type == ScenarioObjectiveType::MaxLatency && simulation.metrics().averageLatencySeconds > failure.threshold) {
            state_ = ScenarioRunState::RecoverableFailure;
        }
        if (failure.type == ScenarioObjectiveType::MaxErrorRate && simulation.metrics().timeoutRatePerSecond > failure.threshold) {
            state_ = ScenarioRunState::RecoverableFailure;
        }
    }

    for (const auto& objective : scenario_.objectives) {
        if (objective.type == ScenarioObjectiveType::SurviveDuration && elapsedSeconds_ >= objective.durationSeconds) {
            state_ = ScenarioRunState::Succeeded;
        }
    }
}
