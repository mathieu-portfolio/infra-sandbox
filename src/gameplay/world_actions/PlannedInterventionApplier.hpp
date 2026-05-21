#pragma once

#include "app/ScenarioSession.hpp"
#include "ui/core/UiTypes.hpp"

class PlannedInterventionApplier {
public:
    void apply(UiState& state, ScenarioSession& session) const;
};
