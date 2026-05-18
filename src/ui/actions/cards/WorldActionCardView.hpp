#pragma once

#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class WorldActionCardView {
public:
    [[nodiscard]] float preferredDraftHeight(float width, const WorldActionDraft& action) const;

    void drawCompact(Rectangle bounds, const WorldActionDraft& action, bool selected, bool hovered) const;
    void drawDraft(Rectangle bounds, const WorldActionDraft& action, bool selected, bool hovered) const;
};
