#pragma once

#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/core/UiTypes.hpp"

class TimelinePanel {
public:
    void update(UiContext& context, const UiFrameView& view, const UiScenarioView& scenarioView);
    void draw(const UiContext& context, const UiFrameView& view, const UiScenarioView& scenarioView) const;
};
