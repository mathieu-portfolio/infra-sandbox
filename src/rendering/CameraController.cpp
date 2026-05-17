#include "rendering/CameraController.hpp"

#include <algorithm>

void CameraController::handleActions(std::span<const InputEvent> events, float dt)
{
    (void)dt;
    for (const auto& event : events) {
        switch (event.action) {
        case InputAction::PanCamera:
            offset_.x += event.mouseDelta.x / zoom_;
            offset_.y += event.mouseDelta.y / zoom_;
            break;
        case InputAction::ZoomIn:
            zoom_ = std::min(20.0f, zoom_ * 1.12f);
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
