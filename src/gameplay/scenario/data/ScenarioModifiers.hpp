#pragma once

#include "gameplay/events/Event.hpp"
#include "core/scenario/ScenarioEnums.hpp"
#include "gameplay/scenario/data/ScenarioTraffic.hpp"
#include "core/simulation/Mechanics.hpp"

#include <optional>
#include <string>
#include <vector>

struct ScenarioModifierDefinition {
    std::string id;
    std::string displayName;
    std::vector<std::string> tags;
    ScenarioModifierType type = ScenarioModifierType::MobileRefreshWave;
    std::string name;
    std::string description;
    double selectionWeight = 1.0;
    NumericRange selectionWeightRange{1.0, 1.0};
    double trafficMultiplier = 1.0;
    NumericRange trafficMultiplierRange{1.0, 1.0};
    std::optional<double> databaseHeavyShare;
    std::optional<NumericRange> databaseHeavyShareRange;
    std::optional<BurstScenario> burstOverride;
    std::vector<EventDefinition> events;
};
