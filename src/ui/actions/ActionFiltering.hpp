#pragma once

#include "ui/actions/cards/ActionCardModel.hpp"

#include <string>
#include <vector>

namespace actions_ui {
std::string categoryFilterLabel(const std::vector<ActionCardModel>& cards, int index);
}
