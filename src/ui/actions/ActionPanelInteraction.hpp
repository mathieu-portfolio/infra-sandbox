#pragma once

#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/core/UiTypes.hpp"

class ActionPanelInteraction {
public:
    void update(UiContext& context, const UiFrameView& view) const;
};
