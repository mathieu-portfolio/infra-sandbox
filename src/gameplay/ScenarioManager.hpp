#pragma once

#include "gameplay/Event.hpp"
#include "gameplay/Scenario.hpp"
#include "simulation/Simulation.hpp"

#include <cstdint>
#include <string>
#include <vector>

class ScenarioManager {
public:
    explicit ScenarioManager(ScenarioDefinition scenario = Scenario::createDefault());

    void load(ScenarioDefinition scenario);
    void createRun(std::uint32_t seed);
    void reset();
    void update(double dt, Simulation& simulation);
    void setCalendarProgressionScale(double calendarDaysPerSimulationSecond);

    [[nodiscard]] const ScenarioDefinition& definition() const;
    [[nodiscard]] const ScenarioDefinition& staticDefinition() const;
    [[nodiscard]] const ScenarioRun& run() const;
    [[nodiscard]] const ScenarioPhase* currentPhase() const;
    [[nodiscard]] std::string objectiveSummary() const;
    [[nodiscard]] std::string focusSummary() const;
    [[nodiscard]] std::string availableMechanicsSummary() const;
    [[nodiscard]] const EventManager& eventManager() const;
    [[nodiscard]] ScenarioRunState state() const;
    [[nodiscard]] double elapsedSeconds() const;
    [[nodiscard]] const GameplayDuration& currentTransitionDuration() const;
    [[nodiscard]] std::string visibleCalendarLabel(const Simulation& simulation) const;
    [[nodiscard]] std::string archetypeSummary() const;
    [[nodiscard]] std::string progressionTierSummary() const;
    [[nodiscard]] std::string activeModifiersSummary() const;
    [[nodiscard]] const ProgressionState& progressionState() const;
    [[nodiscard]] bool isScenarioUnlocked(const ScenarioDefinition& scenario) const;
    void notifyActionTriggered(MechanicType mechanic);
    [[nodiscard]] const SandboxControls& sandboxControls() const;
    void setSandboxTrafficMultiplier(double multiplier);
    void setSandboxLatencyMultiplier(double multiplier);
    void setSandboxQueueBuildup(bool enabled);
    void setSandboxSeed(std::uint32_t seed);
    void injectSandboxEvent(const std::string& id, const Simulation& simulation);
    void clearSandboxEvents();

private:
    [[nodiscard]] ScenarioDefinition createActiveDefinition(std::uint32_t seed) const;
    [[nodiscard]] std::vector<ScenarioModifierDefinition> selectModifiers(std::uint32_t seed) const;
    void applyProgressionTierFilters(ScenarioDefinition& definition) const;
    void applyModifier(ScenarioDefinition& definition, const ScenarioModifierDefinition& modifier) const;
    void applyPhaseToSimulation(Simulation& simulation) const;
    void updateState(const Simulation& simulation);
    void initializeScenarioRunState();
    [[nodiscard]] bool isObjectiveActive(const ScenarioObjective& objective) const;
    [[nodiscard]] bool isObjectiveCompleted(const ScenarioObjective& objective) const;
    [[nodiscard]] bool objectiveSatisfied(const ScenarioObjective& objective, const Simulation& simulation) const;
    void completeObjective(const ScenarioObjective& objective);
    void applyReward(const ObjectiveReward& reward);
    void unlockIntervention(MechanicType mechanic);
    void completeScenario();

    ScenarioDefinition scenario_;
    ScenarioRun run_;
    ProgressionState progressionState_;
    SandboxControls sandboxControls_{};
    std::vector<MechanicType> actionsTriggered_;
    EventManager eventManager_;
    double calendarDaysPerSimulationSecond_ = 1.0 / 86400.0;
    std::uint32_t nextSeed_ = 1;
};
