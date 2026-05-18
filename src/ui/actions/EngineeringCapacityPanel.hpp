#pragma once

#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class EngineeringCapacityPanel {
public:
    void draw(Rectangle bounds, const UiState& state) const;
};
