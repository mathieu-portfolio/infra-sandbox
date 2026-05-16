#pragma once

#include "gameplay/Event.hpp"
#include "gameplay/Scenario.hpp"
#include "simulation/Simulation.hpp"

#include <string>

enum class ScenarioRunState {
    Running,
    Succeeded,
    RecoverableFailure
};

class ScenarioManager {
public:
    explicit ScenarioManager(ScenarioDefinition scenario = Scenario::createDefault());

    void load(ScenarioDefinition scenario);
    void reset();
    void update(double dt, Simulation& simulation);

    [[nodiscard]] const ScenarioDefinition& definition() const;
    [[nodiscard]] const ScenarioPhase* currentPhase() const;
    [[nodiscard]] std::string objectiveSummary() const;
    [[nodiscard]] std::string focusSummary() const;
    [[nodiscard]] std::string availableMechanicsSummary() const;
    [[nodiscard]] const EventManager& eventManager() const;
    [[nodiscard]] ScenarioRunState state() const;
    [[nodiscard]] double elapsedSeconds() const;

private:
    void applyPhaseToSimulation(Simulation& simulation) const;
    void updateState(const Simulation& simulation);

    ScenarioDefinition scenario_;
    ScenarioRunState state_ = ScenarioRunState::Running;
    EventManager eventManager_;
    double elapsedSeconds_ = 0.0;
    int currentPhaseIndex_ = -1;
};
