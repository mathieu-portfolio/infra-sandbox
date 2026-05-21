#pragma once

#include "ui/actions/cards/ActionCardModel.hpp"

#include <string>
#include <vector>

namespace actions_ui {
std::vector<std::string> categoryFilterLabels(const std::vector<ActionCardModel>& cards);
std::string categoryFilterLabel(const std::vector<ActionCardModel>& cards, int index);
bool actionMatchesCategory(const ActionCardModel& card, const std::vector<std::string>& labels, int index);
}
