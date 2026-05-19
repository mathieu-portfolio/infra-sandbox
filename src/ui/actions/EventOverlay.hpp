#pragma once

#include "gameplay/ScenarioManager.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class EventOverlay {
public:
    [[nodiscard]] static Rectangle acknowledgeButtonBounds(Rectangle overlay);
    [[nodiscard]] static Rectangle overlayBounds(int screenWidth, int screenHeight);

    void draw(const UiContext& context, const ScenarioManager& scenarioManager) const;
};
