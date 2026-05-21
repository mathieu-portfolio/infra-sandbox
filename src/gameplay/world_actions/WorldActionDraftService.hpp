#pragma once

#include "app/ScenarioSession.hpp"
#include "ui/core/UiTypes.hpp"

class WorldActionDraftService {
public:
    void generateDraft(UiState& state, ScenarioSession& session) const;
};
