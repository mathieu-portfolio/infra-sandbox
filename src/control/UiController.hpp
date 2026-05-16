#pragma once

#include "input/InputAction.hpp"
#include "ui/UiTypes.hpp"

#include <span>

class UiController {
public:
    void handleActions(std::span<const InputEvent> events, UiState& state);
};
