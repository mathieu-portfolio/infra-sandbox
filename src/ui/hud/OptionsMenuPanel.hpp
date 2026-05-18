#pragma once

#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class OptionsMenuPanel {
public:
    bool update(UiContext& context, Vector2 mouse);
    void drawButton(const UiContext& context) const;
    void drawMenu(const UiContext& context) const;
    [[nodiscard]] Rectangle menuBounds(const UiContext& context) const;
};
