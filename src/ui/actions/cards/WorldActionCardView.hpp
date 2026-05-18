#pragma once

#include "ui/UiTypes.hpp"

#include "raylib.h"

class WorldActionCardView {
public:
    void drawCompact(Rectangle bounds, const WorldActionDraft& action, bool selected, bool hovered) const;
    void drawDraft(Rectangle bounds, const WorldActionDraft& action, bool selected, bool hovered) const;
};
