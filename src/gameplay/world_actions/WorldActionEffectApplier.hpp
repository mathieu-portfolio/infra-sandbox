#pragma once

#include "app/ScenarioSession.hpp"
#include "ui/core/UiTypes.hpp"

class WorldActionEffectApplier {
public:
    void applySelectedWorldAction(UiState& state, ScenarioSession& session) const;
};
