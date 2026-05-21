#pragma once

#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/core/UiTypes.hpp"
#include "ui/actions/EngineeringCapacityPanel.hpp"
#include "ui/actions/EventOverlay.hpp"
#include "ui/actions/WorldActionOverlay.hpp"

class ActionPanel {
public:
    void update(UiContext& context, const UiFrameView& view);
    void draw(const UiContext& context, const UiFrameView& view) const;
    void drawPlanningOverlays(const UiContext& context, const UiScenarioView& scenarioView) const;

private:
    EngineeringCapacityPanel engineeringCapacityPanel_{};
    EventOverlay eventOverlay_{};
    WorldActionOverlay worldActionOverlay_{};
};
