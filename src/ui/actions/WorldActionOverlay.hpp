#pragma once

#include "ui/core/UiTypes.hpp"

#include "raylib.h"

class WorldActionOverlay {
public:
    [[nodiscard]] static Rectangle toggleBounds(int screenWidth);
    [[nodiscard]] static Rectangle overlayBounds(int screenWidth, int screenHeight);
    [[nodiscard]] static Rectangle overlayBounds(int screenWidth, int screenHeight, const std::vector<WorldActionDraft>& drafts);
    [[nodiscard]] static Rectangle draftCardBounds(Rectangle overlay, int index, int count);

    void draw(const UiContext& context) const;
};
