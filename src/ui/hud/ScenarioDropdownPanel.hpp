#pragma once

#include "gameplay/ScenarioManager.hpp"
#include "ui/UiTypes.hpp"

#include "raylib.h"

class ScenarioDropdownPanel {
public:
    bool update(UiContext& context, const ScenarioManager& scenarioManager, Vector2 mouse);
    void drawField(const UiContext& context, const ScenarioManager& scenarioManager) const;
    void drawMenu(const UiContext& context, const ScenarioManager& scenarioManager) const;
    [[nodiscard]] Rectangle menuBounds(const UiContext& context) const;
};
