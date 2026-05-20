#pragma once

#include "content/loading/ContentPackManager.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class PackDropdownPanel {
public:
    bool update(UiContext& context, const content::ContentPackManager& packManager, Vector2 mouse);
    void drawField(const UiContext& context, const content::ContentPackManager& packManager) const;
    void drawMenu(const UiContext& context, const content::ContentPackManager& packManager) const;
    [[nodiscard]] Rectangle menuBounds(const UiContext& context, const content::ContentPackManager& packManager) const;
};
