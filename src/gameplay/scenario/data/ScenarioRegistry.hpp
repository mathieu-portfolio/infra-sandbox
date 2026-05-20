#pragma once

#include "gameplay/scenario/data/ScenarioDefinition.hpp"
#include "gameplay/scenario/data/ScenarioDuration.hpp"
#include "gameplay/scenario/data/ScenarioEnums.hpp"

#include <vector>

class Scenario {
public:
    static ScenarioDefinition createDefault();
};

class ScenarioRegistry {
public:
    static std::vector<ScenarioDefinition> createAll();
    static ScenarioDefinition singleServiceOverload();
    static ScenarioDefinition databaseBottleneck();
    static ScenarioDefinition burstTraffic();
};

const char* progressionTierName(ProgressionTier tier);
const char* scenarioArchetypeName(ScenarioArchetype archetype);
const char* engineeringDomainName(EngineeringDomain domain);
const char* gameplayDurationUnitName(GameplayDurationUnit unit);
double gameplayDurationCalendarDays(const GameplayDuration& duration);
