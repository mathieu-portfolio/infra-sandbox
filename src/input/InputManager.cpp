#include "input/InputManager.hpp"

#include "raylib.h"

#include <utility>

InputMap::InputMap()
{
    bindings_ = {
        {InputAction::ResetSimulation, InputDevice::Keyboard, KEY_R, InputTrigger::Pressed},
        {InputAction::ResetCamera, InputDevice::Keyboard, KEY_HOME, InputTrigger::Pressed},
        {InputAction::Select, InputDevice::MouseButton, MOUSE_BUTTON_LEFT, InputTrigger::Pressed},
        {InputAction::ClearSelection, InputDevice::Keyboard, KEY_ESCAPE, InputTrigger::Pressed},
        {InputAction::ToggleDebugUI, InputDevice::Keyboard, KEY_F9, InputTrigger::Pressed},
        {InputAction::ToggleMetricsUI, InputDevice::Keyboard, KEY_F10, InputTrigger::Pressed},
    };
}

const std::vector<InputBinding>& InputMap::bindings() const
{
    return bindings_;
}

InputManager::InputManager(InputMap inputMap)
    : inputMap_(std::move(inputMap))
{
}

std::vector<InputEvent> InputManager::poll()
{
    std::vector<InputEvent> events;
    const Vector2 mouse = GetMousePosition();

    for (const auto& binding : inputMap_.bindings()) {
        bool active = false;
        if (binding.device == InputDevice::Keyboard) {
            active = binding.trigger == InputTrigger::Pressed ? IsKeyPressed(binding.code) : IsKeyDown(binding.code);
        } else if (binding.device == InputDevice::MouseButton) {
            active = binding.trigger == InputTrigger::Pressed ? IsMouseButtonPressed(binding.code) : IsMouseButtonDown(binding.code);
        }

        if (active) {
            events.push_back({
                .action = binding.action,
                .phase = binding.trigger == InputTrigger::Pressed ? InputPhase::Pressed : InputPhase::Held,
                .mousePosition = mouse,
            });
        }
    }

    const Vector2 mouseDelta = GetMouseDelta();
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)) {
        events.push_back({
            .action = InputAction::PanCamera,
            .phase = InputPhase::Held,
            .mousePosition = mouse,
            .mouseDelta = mouseDelta,
        });
    }

    const float wheel = GetMouseWheelMove();
    if (wheel > 0.0f) {
        events.push_back({InputAction::ZoomIn, InputPhase::Pressed, mouse});
    } else if (wheel < 0.0f) {
        events.push_back({InputAction::ZoomOut, InputPhase::Pressed, mouse});
    }

    return events;
}

const InputMap& InputManager::inputMap() const
{
    return inputMap_;
}
