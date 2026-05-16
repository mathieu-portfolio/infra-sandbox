#pragma once

#include "input/InputAction.hpp"

#include <vector>

enum class InputDevice {
    Keyboard,
    MouseButton
};

enum class InputTrigger {
    Pressed,
    Held
};

struct InputBinding {
    InputAction action = InputAction::PauseSimulation;
    InputDevice device = InputDevice::Keyboard;
    int code = 0;
    InputTrigger trigger = InputTrigger::Pressed;
};

class InputMap {
public:
    InputMap();

    [[nodiscard]] const std::vector<InputBinding>& bindings() const;

private:
    std::vector<InputBinding> bindings_;
};

class InputManager {
public:
    explicit InputManager(InputMap inputMap = {});

    [[nodiscard]] std::vector<InputEvent> poll();
    [[nodiscard]] const InputMap& inputMap() const;

private:
    InputMap inputMap_;
};
