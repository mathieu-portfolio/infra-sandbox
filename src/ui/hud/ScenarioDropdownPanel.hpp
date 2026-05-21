#pragma once

#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class ScenarioDropdownPanel {
public:
    bool update(UiContext& context, const UiScenarioView& scenarioView, Vector2 mouse);
    void drawField(const UiContext& context, const UiScenarioView& scenarioView) const;
    void drawMenu(const UiContext& context, const UiScenarioView& scenarioView) const;
    [[nodiscard]] Rectangle menuBounds(const UiContext& context) const;
};
