#pragma once

#include "gameplay/scenario/data/ScenarioDefinition.hpp"
#include "core/scenario/ScenarioEnums.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct ScenarioRun {
    std::uint32_t seed = 0;
    std::vector<ScenarioModifierDefinition> selectedModifiers;
    ScenarioRunState state = ScenarioRunState::Running;
    double elapsedSeconds = 0.0;
    int turnNumber = 0;
    double turnElapsedSeconds = 0.0;
    double calendarElapsedDays = 0.0;
    double objectiveProgress = 0.0;
    int currentPhaseIndex = -1;
    ScenarioDefinition activeDefinition;
    std::vector<std::string> activeObjectiveIds;
    std::vector<std::string> completedObjectiveIds;
    std::vector<MechanicType> unlockedInterventions;
    std::vector<std::string> unlockedScenarioIds;
    std::vector<std::string> unlockedMetrics;
    std::vector<std::string> unlockedOverlays;
    std::vector<std::string> feedbackMessages;
};

struct SandboxControls {
    double trafficMultiplier = 1.0;
    double latencyMultiplier = 1.0;
    double databaseCapacityMultiplier = 1.0;
    bool queueBuildup = false;
    std::uint32_t seed = 1;
};
