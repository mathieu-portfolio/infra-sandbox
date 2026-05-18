#pragma once

#include "ui/actions/cards/ActionCardModel.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

#include <string>
#include <vector>

namespace actions_ui::cards {

struct CardVisualState {
    bool highlighted = false;
    bool selected = false;
    bool hovered = false;
};

[[nodiscard]] const char* iconForActionCard(const ActionCardModel& card);
[[nodiscard]] std::string categoryLabel(const ActionCardModel& card);
[[nodiscard]] int actionPointCost(const std::vector<EngineeringCost>& costs);
[[nodiscard]] std::vector<std::string> usefulPoints(const ActionCardModel& card);
[[nodiscard]] std::vector<std::string> worsenPoints(const ActionCardModel& card);

void drawCardChrome(Rectangle bounds, CardVisualState state);
void drawChip(Rectangle bounds, const std::string& label, Color fill, Color text);

}
