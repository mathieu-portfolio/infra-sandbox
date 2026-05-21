#pragma once

#include "content/ContentRegistry.hpp"
#include "gameplay/events/Event.hpp"

#include <string>

namespace content::loading {

[[nodiscard]] ProgressionTier progressionTierFromId(const std::string& id);
[[nodiscard]] ScenarioArchetype archetypeFromId(const std::string& id);
[[nodiscard]] GameplayDurationUnit durationUnitFromId(const std::string& id);
[[nodiscard]] EducationalFocus focusFromId(const std::string& id);
[[nodiscard]] PressureCategory pressureFromId(const std::string& id);
[[nodiscard]] MechanicType mechanicFromId(const std::string& id);
[[nodiscard]] bool knownMechanicId(const std::string& id);
[[nodiscard]] TopologyMutationType mutationFromId(const std::string& id);
[[nodiscard]] bool knownMutationId(const std::string& id);
[[nodiscard]] NodeType nodeTypeFromId(const std::string& id);
[[nodiscard]] bool knownNodeTypeId(const std::string& id);
[[nodiscard]] EngineeringDomain engineeringDomainFromId(const std::string& id);
[[nodiscard]] bool knownEngineeringDomainId(const std::string& id);
[[nodiscard]] TrafficProfileType trafficTypeFromId(const std::string& id);
[[nodiscard]] ScenarioObjectiveType objectiveTypeFromId(const std::string& id);
[[nodiscard]] ObjectiveConditionType objectiveConditionFromId(const std::string& id);
[[nodiscard]] ObjectiveRewardType objectiveRewardFromId(const std::string& id);
[[nodiscard]] EventCategory eventCategoryFromId(const std::string& id);
[[nodiscard]] EventMoment eventMomentFromId(const std::string& id);
[[nodiscard]] EventTriggerType triggerTypeFromId(const std::string& id);
[[nodiscard]] EventMetric metricFromId(const std::string& id);
[[nodiscard]] bool knownMetricId(const std::string& id);
[[nodiscard]] EventEffectType effectTypeFromId(const std::string& id);
[[nodiscard]] EventLocationScope eventLocationScopeFromId(const std::string& id);
[[nodiscard]] ScenarioModifierType modifierTypeFromId(const std::string& id);

} // namespace content::loading
