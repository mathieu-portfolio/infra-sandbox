#pragma once

#include "simulation/Simulation.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "ui/DebugPanel.hpp"
#include "ui/HudPanel.hpp"
#include "ui/InterventionPanel.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/OverlayController.hpp"
#include "ui/SelectionPanel.hpp"
#include "ui/TimelinePanel.hpp"
#include "ui/UiTypes.hpp"

class UiManager {
public:
    void update(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused);
    void draw(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused) const;

    [[nodiscard]] const UiState& state() const;
    [[nodiscard]] UiState& state();
    [[nodiscard]] const OverlayController& overlayController() const;

private:
    void updateActionObservations(const Simulation& simulation);

    UiState state_{};
    OverlayController overlayController_{};
    HudPanel hudPanel_{};
    MetricsPanel metricsPanel_{};
    SelectionPanel selectionPanel_{};
    InterventionPanel interventionPanel_{};
    TimelinePanel timelinePanel_{};
    DebugPanel debugPanel_{};
};
