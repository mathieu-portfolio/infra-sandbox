#include "ui/viewmodels/UiFrameView.hpp"

#include "gameplay/scenario/ScenarioManager.hpp"
#include "simulation/core/Simulation.hpp"

#include <algorithm>
#include <cstddef>

const NodePressure* UiPressureAnalysisView::pressureForNode(int nodeId) const
{
    if (snapshot == nullptr) {
        return nullptr;
    }
    const auto it = std::find_if(snapshot->nodes.begin(), snapshot->nodes.end(), [nodeId](const NodePressure& pressure) {
        return pressure.nodeId == nodeId;
    });
    return it != snapshot->nodes.end() ? &(*it) : nullptr;
}

bool UiScenarioView::isScenarioUnlocked(const ScenarioDefinition& definition) const
{
    const auto it = std::find_if(options.begin(), options.end(), [&definition](const UiScenarioOptionView& option) {
        return option.definition.id == definition.id;
    });
    return it != options.end() ? it->unlocked : false;
}

UiFrameView buildUiFrameView(const Simulation& simulation, const ScenarioManager& scenarioManager)
{
    UiFrameView view{};
    view.graphValue = &simulation.graph();
    view.requestsValue = &simulation.requests();
    view.runtimeSystemsValue = &simulation.runtimeSystems();
    view.metricsValue = simulation.metrics();
    view.pressureValue = simulation.pressure();
    view.pressureAnalysisValue.snapshot = &view.pressureValue;
    view.timeSecondsValue = simulation.timeSeconds();
    view.simulationSpeedValue = simulation.simulationSpeed();
    view.mechanicAllowed = [&simulation](MechanicType mechanic) { return simulation.isMechanicAllowed(mechanic); };
    view.canScaleNodeValue = [&simulation](int nodeId, int maxScaleLevel) { return simulation.canScaleNode(nodeId, maxScaleLevel); };
    view.scaleLevelForNodeValue = [&simulation](int nodeId) { return simulation.scaleLevelForNode(nodeId); };
    view.maxScaleLevelForNodeValue = [&simulation](int nodeId, int contentMaxScaleLevel) { return simulation.maxScaleLevelForNode(nodeId, contentMaxScaleLevel); };
    view.hasAnyRegionCapacityValue = [&simulation](int slots) { return simulation.hasAnyRegionCapacity(slots); };

    view.scenario.definitionValue = scenarioManager.definition();
    view.scenario.staticDefinitionValue = scenarioManager.staticDefinition();
    view.scenario.runValue = scenarioManager.run();
    view.scenario.objectiveSummaryValue = scenarioManager.objectiveSummary();
    view.scenario.archetypeSummaryValue = scenarioManager.archetypeSummary();
    view.scenario.focusSummaryValue = scenarioManager.focusSummary();
    view.scenario.activeModifiersSummaryValue = scenarioManager.activeModifiersSummary();
    view.scenario.visibleCalendarLabelValue = scenarioManager.visibleCalendarLabel(simulation);
    if (const ScenarioPhase* phase = scenarioManager.currentPhase()) {
        view.scenario.hasCurrentPhaseValue = true;
        view.scenario.currentPhaseValue = *phase;
    }
    const auto& recentEvents = scenarioManager.eventManager().recentEvents();
    view.scenario.recentEventsValue.assign(recentEvents.begin(), recentEvents.end());

    const auto scenarios = ScenarioRegistry::createAll();
    view.scenario.options.reserve(scenarios.size());
    for (const auto& scenario : scenarios) {
        view.scenario.options.push_back({
            .definition = scenario,
            .unlocked = scenarioManager.isScenarioUnlocked(scenario),
            .archetypeSummary = scenario.id == scenarioManager.staticDefinition().id ? scenarioManager.archetypeSummary() : std::string{},
            .focusSummary = scenario.id == scenarioManager.staticDefinition().id ? scenarioManager.focusSummary() : std::string{},
            .modifiersSummary = scenario.id == scenarioManager.staticDefinition().id ? scenarioManager.activeModifiersSummary() : std::string{},
        });
    }
    return view;
}
