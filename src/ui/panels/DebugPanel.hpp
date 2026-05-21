#pragma once

#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/core/UiTypes.hpp"

class DebugPanel {
public:
    void update(UiContext& context, const UiFrameView& view);
    void draw(const UiContext& context, const UiFrameView& view) const;
};
