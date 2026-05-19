#pragma once

#include "app/ScenarioSession.hpp"
#include "ui/core/UiTypes.hpp"

class WorldActionController {
public:
    void generateDraft(UiState& state, ScenarioSession& session) const;
    void clearPlan(UiState& state) const;
    void applyPlan(UiState& state, ScenarioSession& session) const;
    void applyPlannedInterventions(UiState& state, ScenarioSession& session) const;
};
