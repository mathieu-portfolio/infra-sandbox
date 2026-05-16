#pragma once

#include "gameplay/Event.hpp"
#include "gameplay/Scenario.hpp"
#include "simulation/Simulation.hpp"

#include <cstdint>
#include <string>

class ScenarioManager {
public:
    explicit ScenarioManager(ScenarioDefinition scenario = Scenario::createDefault());

    void load(ScenarioDefinition scenario);
    void createRun(std::uint32_t seed);
    void reset();
    void update(double dt, Simulation& simulation);

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
    [[nodiscard]] std::string archetypeSummary() const;
    [[nodiscard]] std::string progressionTierSummary() const;
    [[nodiscard]] std::string activeModifiersSummary() const;

private:
    [[nodiscard]] ScenarioDefinition createActiveDefinition(std::uint32_t seed) const;
    [[nodiscard]] std::vector<ScenarioModifierDefinition> selectModifiers(std::uint32_t seed) const;
    void applyProgressionTierFilters(ScenarioDefinition& definition) const;
    void applyModifier(ScenarioDefinition& definition, const ScenarioModifierDefinition& modifier) const;
    void applyPhaseToSimulation(Simulation& simulation) const;
    void updateState(const Simulation& simulation);

    ScenarioDefinition scenario_;
    ScenarioRun run_;
    EventManager eventManager_;
    std::uint32_t nextSeed_ = 1;
};
