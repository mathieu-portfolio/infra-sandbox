#pragma once

#include "gameplay/Scenario.hpp"
#include "gameplay/events/Event.hpp"
#include "simulation/core/RuntimeSystems.hpp"
#include "simulation/metrics/Metrics.hpp"
#include "simulation/metrics/PressureAnalysis.hpp"
#include "simulation/requests/Request.hpp"
#include "simulation/topology/InfrastructureGraph.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class ScenarioManager;
class Simulation;

template <typename T>
struct ValueRef {
    const T* value = nullptr;
    [[nodiscard]] const T& get() const { return *value; }
};

struct UiPressureAnalysisView {
    const PressureSnapshot* snapshot = nullptr;

    [[nodiscard]] const NodePressure* pressureForNode(int nodeId) const;
};

struct UiScenarioOptionView {
    ScenarioDefinition definition;
    bool unlocked = false;
    std::string archetypeSummary;
    std::string focusSummary;
    std::string modifiersSummary;
};

struct UiScenarioView {
    ScenarioDefinition definitionValue;
    ScenarioDefinition staticDefinitionValue;
    ScenarioRun runValue;
    std::string objectiveSummaryValue;
    std::string archetypeSummaryValue;
    std::string focusSummaryValue;
    std::string activeModifiersSummaryValue;
    std::string visibleCalendarLabelValue;
    bool hasCurrentPhaseValue = false;
    ScenarioPhase currentPhaseValue{};
    std::vector<UiScenarioOptionView> options;
    std::vector<EventLogEntry> recentEventsValue;

    [[nodiscard]] const ScenarioDefinition& definition() const { return definitionValue; }
    [[nodiscard]] const ScenarioDefinition& staticDefinition() const { return staticDefinitionValue; }
    [[nodiscard]] const ScenarioRun& run() const { return runValue; }
    [[nodiscard]] const std::string& objectiveSummary() const { return objectiveSummaryValue; }
    [[nodiscard]] const std::string& archetypeSummary() const { return archetypeSummaryValue; }
    [[nodiscard]] const std::string& focusSummary() const { return focusSummaryValue; }
    [[nodiscard]] const std::string& activeModifiersSummary() const { return activeModifiersSummaryValue; }
    [[nodiscard]] const std::string& visibleCalendarLabel() const { return visibleCalendarLabelValue; }
    [[nodiscard]] const ScenarioPhase* currentPhase() const { return hasCurrentPhaseValue ? &currentPhaseValue : nullptr; }
    [[nodiscard]] const std::vector<EventLogEntry>& recentEvents() const { return recentEventsValue; }
    [[nodiscard]] bool isScenarioUnlocked(const ScenarioDefinition& definition) const;
};

struct UiFrameView {
    const InfrastructureGraph* graphValue = nullptr;
    const std::unordered_map<std::uint64_t, Request>* requestsValue = nullptr;
    const RuntimeSystems* runtimeSystemsValue = nullptr;
    MetricsSnapshot metricsValue{};
    PressureSnapshot pressureValue{};
    UiScenarioView scenario;
    double timeSecondsValue = 0.0;
    double simulationSpeedValue = 1.0;
    std::function<bool(MechanicType)> mechanicAllowed;
    std::function<bool(int, int)> canScaleNodeValue;
    std::function<int(int)> scaleLevelForNodeValue;
    std::function<int(int, int)> maxScaleLevelForNodeValue;
    std::function<bool(int)> hasAnyRegionCapacityValue;

    [[nodiscard]] const InfrastructureGraph& graph() const { return *graphValue; }
    [[nodiscard]] const std::unordered_map<std::uint64_t, Request>& requests() const { return *requestsValue; }
    [[nodiscard]] const RuntimeSystems& runtimeSystems() const { return *runtimeSystemsValue; }
    [[nodiscard]] const MetricsSnapshot& metrics() const { return metricsValue; }
    [[nodiscard]] const PressureSnapshot& pressure() const { return pressureValue; }
    [[nodiscard]] UiPressureAnalysisView pressureAnalysis() const { return {.snapshot = &pressureValue}; }
    [[nodiscard]] double timeSeconds() const { return timeSecondsValue; }
    [[nodiscard]] double simulationSpeed() const { return simulationSpeedValue; }
    [[nodiscard]] bool isMechanicAllowed(MechanicType mechanic) const { return mechanicAllowed ? mechanicAllowed(mechanic) : false; }
    [[nodiscard]] bool canScaleNode(int nodeId, int maxScaleLevel) const { return canScaleNodeValue ? canScaleNodeValue(nodeId, maxScaleLevel) : false; }
    [[nodiscard]] int scaleLevelForNode(int nodeId) const { return scaleLevelForNodeValue ? scaleLevelForNodeValue(nodeId) : 0; }
    [[nodiscard]] int maxScaleLevelForNode(int nodeId, int contentMaxScaleLevel) const { return maxScaleLevelForNodeValue ? maxScaleLevelForNodeValue(nodeId, contentMaxScaleLevel) : contentMaxScaleLevel; }
    [[nodiscard]] bool hasAnyRegionCapacity(int slots) const { return hasAnyRegionCapacityValue ? hasAnyRegionCapacityValue(slots) : false; }
};

[[nodiscard]] UiFrameView buildUiFrameView(const Simulation& simulation, const ScenarioManager& scenarioManager);
