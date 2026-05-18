#pragma once

#include "simulation/Simulation.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "ui/DebugPanel.hpp"
#include "ui/HudPanel.hpp"
#include "ui/actions/ActionPanel.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/OverlayController.hpp"
#include "ui/SelectionPanel.hpp"
#include "ui/TimelinePanel.hpp"
#include "ui/core/UiTypes.hpp"

class UiManager {
public:
    void update(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused);
    void draw(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused) const;

    [[nodiscard]] const UiState& state() const;
    [[nodiscard]] UiState& state();
    [[nodiscard]] const OverlayController& overlayController() const;
    void releaseResources();

private:
    void updateActionObservations(const Simulation& simulation);
    void updateMetricHistory(const Simulation& simulation);

    UiState state_{};
    OverlayController overlayController_{};
    HudPanel hudPanel_{};
    MetricsPanel metricsPanel_{};
    SelectionPanel selectionPanel_{};
    ActionPanel actionPanel_{};
    TimelinePanel timelinePanel_{};
    DebugPanel debugPanel_{};
};
