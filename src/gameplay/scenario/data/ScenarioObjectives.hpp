#pragma once

#include "core/scenario/ScenarioEnums.hpp"
#include "simulation/metrics/PressureAnalysis.hpp"

#include <string>
#include <vector>

struct ObjectiveReward {
    ObjectiveRewardType type = ObjectiveRewardType::EmitFeedback;
    std::string id;
    std::string message;
};

struct ScenarioObjective {
    std::string id;
    std::string displayName;
    std::string description;
    std::vector<std::string> tags;
    ScenarioObjectiveType type = ScenarioObjectiveType::SurviveDuration;
    ObjectiveConditionType conditionType = ObjectiveConditionType::SurviveDuration;
    std::string conditionMetric;
    PressureCategory pressure = PressureCategory::None;
    std::string targetNodeId;
    std::string summary;
    double threshold = 0.0;
    double durationSeconds = 0.0;
    int durationTurns = 0;
    bool startsActive = false;
    std::vector<ObjectiveReward> rewards;
    std::vector<std::string> nextObjectives;
};
