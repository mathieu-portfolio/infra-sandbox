#pragma once

#include "input/InputAction.hpp"

#include "raylib.h"

#include <span>

class CameraController {
public:
    void handleActions(std::span<const InputEvent> events, float dt);
    void update(float dt);

    [[nodiscard]] Vector2 offset() const;
    [[nodiscard]] float zoom() const;
    void reset();

private:
    Vector2 offset_{0.0f, 0.0f};
    float zoom_ = 1.0f;
};
