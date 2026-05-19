#pragma once

#include "gameplay/Scenario.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "simulation/Simulation.hpp"

#include <cstddef>

class ScenarioSession {
public:
    ScenarioSession();
    explicit ScenarioSession(ScenarioDefinition scenario);

    void reloadDefaultScenario();
    void reset();
    bool loadScenario(std::size_t scenarioIndex);
    bool loadScenario(const std::string& scenarioId);
    void regenerateSimulation();

    const ScenarioDefinition& definition() const;
    ScenarioManager& scenarioManager();
    const ScenarioManager& scenarioManager() const;
    Simulation& simulation();
    const Simulation& simulation() const;

private:
    static Simulation makeSimulation(const ScenarioDefinition& scenario);

    ScenarioDefinition scenarioDefinition_;
    ScenarioManager scenarioManager_;
    Simulation simulation_;
};
