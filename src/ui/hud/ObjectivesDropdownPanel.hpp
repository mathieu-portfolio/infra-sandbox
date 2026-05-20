#pragma once

#include "gameplay/scenario/ScenarioManager.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class ObjectivesDropdownPanel {
public:
    bool update(UiContext& context, const ScenarioManager& scenarioManager, Vector2 mouse);
    void drawField(const UiContext& context, const ScenarioManager& scenarioManager) const;
    void drawMenu(const UiContext& context, const ScenarioManager& scenarioManager) const;
    [[nodiscard]] Rectangle menuBounds(const UiContext& context, const ScenarioManager& scenarioManager) const;
};
