#pragma once

#include "app/ScenarioSession.hpp"
#include "gameplay/WorldActionController.hpp"
#include "simulation/Metrics.hpp"
#include "ui/core/UiTypes.hpp"

#include <cstddef>
#include <string>

class GameplayPhaseController {
public:
    void reset();
    void beginScenarioGroundingSimulation(UiState& state, ScenarioSession& session, const WorldActionController& worldActions);
    void applyUiRequests(UiState& state, ScenarioSession& session, const WorldActionController& worldActions);
    void beginTransition(UiState& state, ScenarioSession& session, const WorldActionController& worldActions);
    void updateSimulation(float frameTime, UiState& state, ScenarioSession& session, const WorldActionController& worldActions);

private:
    void finishTransition(UiState& state, ScenarioSession& session, const MetricsSnapshot& beforeMetrics);
    void appendResolutionSummary(UiState& state, std::string summary) const;

    double fixedStepAccumulator_ = 0.0;
    MetricsSnapshot transitionBaseline_{};
    std::size_t transitionEventLogStart_ = 0;
    bool transitionReturnsToObservation_ = false;
};
