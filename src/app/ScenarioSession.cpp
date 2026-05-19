#include "app/ScenarioSession.hpp"

#include "content/ContentRegistry.hpp"

#include <vector>
#include <algorithm>
#include <utility>

ScenarioSession::ScenarioSession()
    : scenarioDefinition_(Scenario::createDefault()),
      scenarioManager_(scenarioDefinition_),
      simulation_(makeSimulation(scenarioManager_.definition()))
{
}

ScenarioSession::ScenarioSession(ScenarioDefinition scenario)
    : scenarioDefinition_(std::move(scenario)),
      scenarioManager_(scenarioDefinition_),
      simulation_(makeSimulation(scenarioManager_.definition()))
{
}

void ScenarioSession::reloadDefaultScenario()
{
    scenarioDefinition_ = Scenario::createDefault();
    scenarioManager_.load(scenarioDefinition_);
    simulation_ = makeSimulation(scenarioManager_.definition());
}

Simulation ScenarioSession::makeSimulation(const ScenarioDefinition& scenario)
{
    return Simulation(scenario, content::ContentRegistry::instance().simulationConfig());
}

void ScenarioSession::reset()
{
    scenarioManager_.reset();
    simulation_ = makeSimulation(scenarioManager_.definition());
}

bool ScenarioSession::loadScenario(std::size_t scenarioIndex)
{
    const std::vector<ScenarioDefinition> scenarios = ScenarioRegistry::createAll();
    if (scenarioIndex >= scenarios.size()) {
        return false;
    }

    scenarioDefinition_ = scenarios[scenarioIndex];
    scenarioManager_.load(scenarioDefinition_);
    simulation_ = makeSimulation(scenarioManager_.definition());
    return true;
}

bool ScenarioSession::loadScenario(const std::string& scenarioId)
{
    const std::vector<ScenarioDefinition> scenarios = ScenarioRegistry::createAll();
    const auto it = std::find_if(scenarios.begin(), scenarios.end(), [&](const ScenarioDefinition& scenario) {
        return scenario.id == scenarioId;
    });
    if (it == scenarios.end()) {
        return false;
    }

    scenarioDefinition_ = *it;
    scenarioManager_.load(scenarioDefinition_);
    simulation_ = makeSimulation(scenarioManager_.definition());
    return true;
}

void ScenarioSession::regenerateSimulation()
{
    simulation_ = makeSimulation(scenarioManager_.definition());
}

const ScenarioDefinition& ScenarioSession::definition() const
{
    return scenarioManager_.definition();
}

ScenarioManager& ScenarioSession::scenarioManager()
{
    return scenarioManager_;
}

const ScenarioManager& ScenarioSession::scenarioManager() const
{
    return scenarioManager_;
}

Simulation& ScenarioSession::simulation()
{
    return simulation_;
}

const Simulation& ScenarioSession::simulation() const
{
    return simulation_;
}
