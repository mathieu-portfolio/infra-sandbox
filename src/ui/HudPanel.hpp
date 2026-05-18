#pragma once

#include "simulation/Simulation.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "ui/hud/ObjectivesDropdownPanel.hpp"
#include "ui/hud/OptionsMenuPanel.hpp"
#include "ui/hud/ScenarioDropdownPanel.hpp"
#include "ui/UiTypes.hpp"

class HudPanel {
public:
    void update(UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager);
    void draw(const UiContext& context, const Simulation& simulation, const ScenarioManager& scenarioManager) const;

private:
    ScenarioDropdownPanel scenarioDropdown_{};
    ObjectivesDropdownPanel objectivesDropdown_{};
    OptionsMenuPanel optionsMenu_{};
};
