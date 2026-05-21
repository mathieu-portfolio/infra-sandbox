#pragma once

#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

#include <vector>

class EventOverlay {
public:
    [[nodiscard]] static Rectangle acknowledgeButtonBounds(Rectangle overlay);
    [[nodiscard]] static Rectangle acknowledgeButtonBounds(int screenWidth, int screenHeight, EventPopupMode mode, const std::vector<EventLogEntry>& events);
    [[nodiscard]] static Rectangle overlayBounds(int screenWidth, int screenHeight);

    void draw(const UiContext& context, const UiScenarioView& scenarioView) const;
};
