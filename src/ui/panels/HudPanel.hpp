#pragma once

#include "ui/viewmodels/UiFrameView.hpp"
#include "content/loading/ContentPackManager.hpp"
#include "ui/viewmodels/UiFrameView.hpp"
#include "ui/hud/ObjectivesDropdownPanel.hpp"
#include "ui/hud/OptionsMenuPanel.hpp"
#include "ui/hud/PackDropdownPanel.hpp"
#include "ui/hud/ScenarioDropdownPanel.hpp"
#include "ui/core/UiTypes.hpp"

class HudPanel {
public:
    void update(UiContext& context, const UiFrameView& view, const UiScenarioView& scenarioView, const content::ContentPackManager& packManager);
    void draw(const UiContext& context, const UiFrameView& view, const UiScenarioView& scenarioView, const content::ContentPackManager& packManager) const;

private:
    PackDropdownPanel packDropdown_{};
    ScenarioDropdownPanel scenarioDropdown_{};
    ObjectivesDropdownPanel objectivesDropdown_{};
    OptionsMenuPanel optionsMenu_{};
};
