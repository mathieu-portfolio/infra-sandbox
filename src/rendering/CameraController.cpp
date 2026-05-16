#include "rendering/CameraController.hpp"

#include <algorithm>

void CameraController::handleActions(std::span<const InputEvent> events, float dt)
{
    const float panSpeed = 360.0f * dt / zoom_;
    for (const auto& event : events) {
        switch (event.action) {
        case InputAction::MoveCameraUp:
            offset_.y += panSpeed;
            break;
        case InputAction::MoveCameraDown:
            offset_.y -= panSpeed;
            break;
        case InputAction::MoveCameraLeft:
            offset_.x += panSpeed;
            break;
        case InputAction::MoveCameraRight:
            offset_.x -= panSpeed;
            break;
        case InputAction::ZoomIn:
            zoom_ = std::min(2.5f, zoom_ * 1.12f);
            break;
        case InputAction::ZoomOut:
            zoom_ = std::max(0.55f, zoom_ / 1.12f);
            break;
        case InputAction::ResetCamera:
            reset();
            break;
        default:
            break;
        }
    }
}

void CameraController::update(float)
{
}

Vector2 CameraController::offset() const
{
    return offset_;
}

float CameraController::zoom() const
{
    return zoom_;
}

void CameraController::reset()
{
    offset_ = {0.0f, 0.0f};
    zoom_ = 1.0f;
}
