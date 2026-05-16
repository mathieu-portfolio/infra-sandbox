#pragma once

#include "input/InputAction.hpp"
#include "ui/UiTypes.hpp"

#include <span>

class OverlayInputController {
public:
    void handleActions(std::span<const InputEvent> events, UiState& state);
};
