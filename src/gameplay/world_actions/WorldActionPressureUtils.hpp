#pragma once

#include "gameplay/Scenario.hpp"

#include <string>

namespace gameplay::world_actions {

[[nodiscard]] PressureState scaledPressureEffect(PressureState effect, double intensity);
[[nodiscard]] bool hasPressureEffect(const PressureState& effect);
[[nodiscard]] std::string pressurePreviewText(const PressureState& effect);

}
