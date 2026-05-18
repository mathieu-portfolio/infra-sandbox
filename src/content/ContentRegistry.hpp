#pragma once

#include "gameplay/Scenario.hpp"
#include "simulation/Mechanics.hpp"
#include "simulation/SimulationConfig.hpp"
#include "simulation/TopologyMutation.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace content {

enum class InterventionKind {
    Mechanic,
    TopologyMutation
};

struct InterventionDefinition {
    std::string id;
    std::string displayName;
    std::string description;
    std::string expectedBenefits;
    std::string tradeoffs;
    std::vector<std::string> positiveEffects;
    std::vector<std::string> negativeEffects;
    std::vector<std::string> pressureShifts;
    std::vector<std::string> categories;
    std::vector<std::string> usefulWhen;
    std::string iconId;
    std::vector<PressureCategory> affectedPressures;
    std::vector<NodeType> targetNodeTypes;
    std::vector<EngineeringCost> engineeringCosts;
    std::string architecturalPattern;
    std::string technologyExample;
    std::vector<std::string> tags;
    InterventionKind kind = InterventionKind::Mechanic;
    MechanicType mechanic = MechanicType::ScaleUp;
    TopologyMutationType mutation = TopologyMutationType::AddCache;
    bool requiresConfirmation = false;
    double complexityCost = 0.0;
    int maxScaleLevel = 3;
    double diminishingReturn = 0.72;
    int regionSlotUsage = 0;
};

struct WorldActionDefinition {
    std::string id;
    std::string displayName;
    std::string description;
    std::vector<std::string> tags;
    std::vector<std::string> categories;
    std::string usefulWhen;
    std::string tradeoffs;
    std::string iconId;
    std::vector<PressureCategory> affectedPressures;
    EngineeringCapacity capacityBonus;
    double pressureResistance = 0.0;
    double eventIntensityMultiplier = 1.0;
    double complexityDelta = 0.0;
    double minIntensity = 0.85;
    double maxIntensity = 1.25;
};

struct ContentLoadResult {
    bool loaded = false;
    std::vector<std::string> errors;
};

class ContentRegistry {
public:
    [[nodiscard]] static ContentRegistry& instance();

    [[nodiscard]] ContentLoadResult loadFromDisk(const std::filesystem::path& root);
    void loadFallbackContent();

    [[nodiscard]] const std::vector<ProgressionTierDefinition>& progressionTiers() const;
    [[nodiscard]] const ProgressionTierDefinition& progressionTier(ProgressionTier tier) const;
    [[nodiscard]] const std::vector<ScenarioDefinition>& scenarios() const;
    [[nodiscard]] const ScenarioDefinition& defaultScenario() const;
    [[nodiscard]] const std::vector<InterventionDefinition>& interventions() const;
    [[nodiscard]] const std::vector<WorldActionDefinition>& worldActions() const;
    [[nodiscard]] const SimulationConfig& simulationConfig() const;
    [[nodiscard]] const std::vector<std::string>& loadErrors() const;
    [[nodiscard]] bool loadedFromContent() const;

private:
    ContentLoadResult loadInternal(const std::filesystem::path& root);
    void validate(ContentLoadResult& result) const;

    std::vector<ProgressionTierDefinition> progressionTiers_;
    std::vector<ScenarioDefinition> scenarios_;
    std::vector<InterventionDefinition> interventions_;
    std::vector<WorldActionDefinition> worldActions_;
    SimulationConfig simulationConfig_;
    std::vector<std::string> loadErrors_;
    bool loadedFromContent_ = false;
};

class ContentManager {
public:
    [[nodiscard]] static ContentLoadResult loadDefaultContent();
};

} // namespace content
