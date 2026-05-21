#pragma once

#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/core/UiTypes.hpp"

class MetricsPanelRenderer {
public:
    void draw(const UiContext& context, const UiFrameView& view) const;
};
