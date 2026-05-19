#pragma once

#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class EngineeringCapacityPanel {
public:
    [[nodiscard]] float measureHeight(const UiState& state) const;
    void draw(Rectangle bounds, const UiState& state) const;
};
