#pragma once

#include "gameplay/Scenario.hpp"
#include "simulation/Mechanics.hpp"
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
    std::vector<std::string> tags;
    InterventionKind kind = InterventionKind::Mechanic;
    MechanicType mechanic = MechanicType::ScaleUp;
    TopologyMutationType mutation = TopologyMutationType::AddCache;
    bool requiresConfirmation = false;
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
    [[nodiscard]] const std::vector<std::string>& loadErrors() const;
    [[nodiscard]] bool loadedFromContent() const;

private:
    ContentLoadResult loadInternal(const std::filesystem::path& root);
    void validate(ContentLoadResult& result) const;

    std::vector<ProgressionTierDefinition> progressionTiers_;
    std::vector<ScenarioDefinition> scenarios_;
    std::vector<InterventionDefinition> interventions_;
    std::vector<std::string> loadErrors_;
    bool loadedFromContent_ = false;
};

class ContentManager {
public:
    [[nodiscard]] static ContentLoadResult loadDefaultContent();
};

} // namespace content
