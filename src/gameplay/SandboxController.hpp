#pragma once

#include "app/ScenarioSession.hpp"
#include "ui/core/UiTypes.hpp"

struct SandboxControllerResult {
    bool resetScenarioRequested = false;
    bool beginTransitionRequested = false;
    bool resetTransitionStateRequested = false;
};

class SandboxController {
public:
    SandboxControllerResult applyRequests(UiState& state, ScenarioSession& session) const;
};
