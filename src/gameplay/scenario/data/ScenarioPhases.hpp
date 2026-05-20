#pragma once

#include "gameplay/scenario/data/ScenarioDuration.hpp"
#include "gameplay/scenario/data/ScenarioTraffic.hpp"
#include "core/simulation/Mechanics.hpp"

#include <optional>
#include <string>
#include <vector>

struct ScenarioPhase {
    std::string name;
    std::string eventMessage;
    double startTimeSeconds = 0.0;
    NumericRange startTimeSecondsRange{0.0, 0.0};
    int startTurn = 0;
    double durationSeconds = 30.0;
    NumericRange durationSecondsRange{30.0, 30.0};
    int durationTurns = 0;
    GameplayDuration transitionDuration;
    double trafficMultiplier = 1.0;
    NumericRange trafficMultiplierRange{1.0, 1.0};
    std::optional<BurstScenario> burstOverride;
    std::vector<MechanicType> unlockMechanics;
};
