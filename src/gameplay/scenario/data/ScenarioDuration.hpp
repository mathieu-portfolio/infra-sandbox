#pragma once

#include "core/scenario/ScenarioEnums.hpp"

#include <string>

struct GameplayDuration {
    double value = 90.0;
    GameplayDurationUnit unit = GameplayDurationUnit::Seconds;
    std::string label = "90 seconds of platform evolution";
    double simulationSeconds = 90.0;
    bool advancesCalendar = true;
};
