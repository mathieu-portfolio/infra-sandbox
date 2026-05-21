#pragma once

#include "gameplay/events/Event.hpp"
#include "gameplay/scenario/data/ScenarioDuration.hpp"
#include "core/scenario/ScenarioEconomy.hpp"
#include "core/scenario/ScenarioEnums.hpp"
#include "gameplay/scenario/data/ScenarioModifiers.hpp"
#include "gameplay/scenario/data/ScenarioObjectives.hpp"
#include "gameplay/scenario/data/ScenarioPhases.hpp"
#include "gameplay/scenario/data/ScenarioProgression.hpp"
#include "gameplay/scenario/data/ScenarioTopology.hpp"
#include "gameplay/scenario/data/ScenarioTraffic.hpp"
#include "core/simulation/Mechanics.hpp"
#include "simulation/metrics/PressureAnalysis.hpp"

#include <string>
#include <unordered_map>
#include <vector>

struct ScenarioDefinition {
    std::string id;
    std::string displayName;
    std::vector<std::string> tags;
    std::string name;
    std::string description;
    ScenarioArchetype archetype = ScenarioArchetype::LocalStartup;
    ProgressionTier minimumTier = ProgressionTier::Foundations;
    std::string topologyTemplateId;
    std::vector<EducationalFocus> educationalFocus;
    std::string initialTopologyTemplate = "default-regional-api";
    std::vector<PressureCategory> guaranteedPressures;
    std::vector<ScenarioModifierDefinition> optionalModifiers;
    std::vector<MechanicType> allowedMechanics;
    std::vector<MechanicType> startingInterventions;
    std::vector<MechanicType> unlockableInterventions;
    std::vector<MechanicType> disabledInterventions;
    std::vector<MechanicType> recommendedMechanics;
    std::vector<std::string> unlocksScenarios;
    std::vector<std::string> requiredCompletedScenarios;
    std::vector<std::string> requiredConceptTags;
    EngineeringCapacity engineeringCapacity;
    bool sandboxLab = false;
    std::vector<NodeScenario> nodes;
    std::vector<LinkScenario> links;
    TrafficProfile trafficProfile;
    RequestTypeScenario requestTypes;
    CacheScenario cache;
    RetryScenario retries;
    BurstScenario bursts;
    std::vector<ScenarioObjective> objectives;
    std::vector<ScenarioObjective> failureConditions;
    std::vector<ScenarioPhase> phases;
    std::vector<EventDefinition> events;
    std::vector<EventDefinition> sandboxEvents;
    PressureState pressureContext;
    std::vector<PressureContextSignal> pressureSignals;
    GameplayDuration turnDuration;
    double requestTimeoutSeconds = 5.5;
    bool proceduralLocations = false;
    double proceduralLocationJitterDegrees = 6.0;
    std::unordered_map<std::string, std::vector<std::string>> proceduralLocationRegions;
};
