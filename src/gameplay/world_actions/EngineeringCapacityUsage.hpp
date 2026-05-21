#pragma once

#include "ui/core/UiTypes.hpp"

#include <string>

namespace gameplay::world_actions {

[[nodiscard]] std::string capacityUsageSummary(const UiState& state);

}
