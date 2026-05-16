#include "input/InputManager.hpp"

#include "raylib.h"

#include <utility>

InputMap::InputMap()
{
    bindings_ = {
        {InputAction::PauseSimulation, InputDevice::Keyboard, KEY_SPACE, InputTrigger::Pressed},
        {InputAction::StepSimulation, InputDevice::Keyboard, KEY_PERIOD, InputTrigger::Pressed},
        {InputAction::ResetSimulation, InputDevice::Keyboard, KEY_R, InputTrigger::Pressed},
        {InputAction::SetSimulationSpeed1x, InputDevice::Keyboard, KEY_ONE, InputTrigger::Pressed},
        {InputAction::SetSimulationSpeed2x, InputDevice::Keyboard, KEY_THREE, InputTrigger::Pressed},
        {InputAction::SetSimulationSpeed5x, InputDevice::Keyboard, KEY_FIVE, InputTrigger::Pressed},
        {InputAction::ThrottleTrafficUp, InputDevice::Keyboard, KEY_UP, InputTrigger::Pressed},
        {InputAction::ThrottleTrafficUp, InputDevice::Keyboard, KEY_EQUAL, InputTrigger::Pressed},
        {InputAction::ThrottleTrafficUp, InputDevice::Keyboard, KEY_KP_ADD, InputTrigger::Pressed},
        {InputAction::ThrottleTrafficDown, InputDevice::Keyboard, KEY_DOWN, InputTrigger::Pressed},
        {InputAction::ThrottleTrafficDown, InputDevice::Keyboard, KEY_MINUS, InputTrigger::Pressed},
        {InputAction::ThrottleTrafficDown, InputDevice::Keyboard, KEY_KP_SUBTRACT, InputTrigger::Pressed},
        {InputAction::MoveCameraUp, InputDevice::Keyboard, KEY_I, InputTrigger::Held},
        {InputAction::MoveCameraDown, InputDevice::Keyboard, KEY_K, InputTrigger::Held},
        {InputAction::MoveCameraLeft, InputDevice::Keyboard, KEY_J, InputTrigger::Held},
        {InputAction::MoveCameraRight, InputDevice::Keyboard, KEY_L, InputTrigger::Held},
        {InputAction::ResetCamera, InputDevice::Keyboard, KEY_HOME, InputTrigger::Pressed},
        {InputAction::Select, InputDevice::MouseButton, MOUSE_BUTTON_LEFT, InputTrigger::Pressed},
        {InputAction::ClearSelection, InputDevice::Keyboard, KEY_ESCAPE, InputTrigger::Pressed},
        {InputAction::OverlayFlow, InputDevice::Keyboard, KEY_F1, InputTrigger::Pressed},
        {InputAction::OverlayLatency, InputDevice::Keyboard, KEY_F2, InputTrigger::Pressed},
        {InputAction::OverlayUtilization, InputDevice::Keyboard, KEY_F3, InputTrigger::Pressed},
        {InputAction::OverlayQueues, InputDevice::Keyboard, KEY_F4, InputTrigger::Pressed},
        {InputAction::OverlayErrors, InputDevice::Keyboard, KEY_F5, InputTrigger::Pressed},
        {InputAction::OverlayReliability, InputDevice::Keyboard, KEY_F6, InputTrigger::Pressed},
        {InputAction::OverlayComplexity, InputDevice::Keyboard, KEY_F7, InputTrigger::Pressed},
        {InputAction::OverlayNone, InputDevice::Keyboard, KEY_F8, InputTrigger::Pressed},
        {InputAction::OverlayBottlenecks, InputDevice::Keyboard, KEY_F11, InputTrigger::Pressed},
        {InputAction::OverlayRetryAmplification, InputDevice::Keyboard, KEY_F12, InputTrigger::Pressed},
        {InputAction::ScaleUp, InputDevice::Keyboard, KEY_A, InputTrigger::Pressed},
        {InputAction::ResetInterventions, InputDevice::Keyboard, KEY_FOUR, InputTrigger::Pressed},
        {InputAction::ToggleCache, InputDevice::Keyboard, KEY_TWO, InputTrigger::Pressed},
        {InputAction::ClearCache, InputDevice::Keyboard, KEY_C, InputTrigger::Pressed},
        {InputAction::ToggleRetries, InputDevice::Keyboard, KEY_T, InputTrigger::Pressed},
        {InputAction::ToggleTrafficBurst, InputDevice::Keyboard, KEY_B, InputTrigger::Pressed},
        {InputAction::ToggleGeoGrid, InputDevice::Keyboard, KEY_G, InputTrigger::Pressed},
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
