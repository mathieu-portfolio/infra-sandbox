#include "content/ContentRegistry.hpp"

#include <algorithm>

namespace content {

ContentRegistry& ContentRegistry::instance()
{
    static ContentRegistry registry;
    return registry;
}

ContentLoadResult ContentRegistry::loadFromDisk(const std::filesystem::path& root)
{
    return loadFromLayers({root});
}

ContentLoadResult ContentRegistry::loadFromLayers(const std::vector<std::filesystem::path>& roots)
{
    ContentLoadResult result = loadInternal(roots);
    if (!result.errors.empty() || scenarios_.empty() || progressionTiers_.empty()) {
        loadErrors_ = result.errors;
        loadFallbackContent();
        result.loaded = false;
        return result;
    }
    validate(result);
    if (!result.errors.empty()) {
        loadErrors_ = result.errors;
        loadFallbackContent();
        result.loaded = false;
        return result;
    }
    loadedFromContent_ = true;
    loadErrors_.clear();
    currentPackPath_ = roots.empty() ? std::filesystem::path{} : roots.back();
    result.loaded = true;
    return result;
}

void ContentRegistry::clearLoadedContent()
{
    packMetadata_ = {};
    currentPackPath_.clear();
    progressionTiers_.clear();
    scenarios_.clear();
    interventions_.clear();
    worldActions_.clear();
    simulationConfig_ = SimulationConfig{};
    loadedFromContent_ = false;
}


const ContentPackMetadata& ContentRegistry::packMetadata() const { return packMetadata_; }
const std::filesystem::path& ContentRegistry::currentPackPath() const { return currentPackPath_; }
const std::vector<ProgressionTierDefinition>& ContentRegistry::progressionTiers() const { return progressionTiers_; }

const ProgressionTierDefinition& ContentRegistry::progressionTier(ProgressionTier tier) const
{
    const auto it = std::find_if(progressionTiers_.begin(), progressionTiers_.end(), [tier](const auto& entry) {
        return entry.tier == tier;
    });
    return it != progressionTiers_.end() ? *it : progressionTiers_.front();
}

const std::vector<ScenarioDefinition>& ContentRegistry::scenarios() const { return scenarios_; }
const ScenarioDefinition& ContentRegistry::defaultScenario() const
{
    if (!packMetadata_.defaultScenarioId.empty()) {
        const auto defaultIt = std::find_if(scenarios_.begin(), scenarios_.end(), [&](const ScenarioDefinition& scenario) {
            return scenario.id == packMetadata_.defaultScenarioId;
        });
        if (defaultIt != scenarios_.end()) {
            return *defaultIt;
        }
    }
    const auto it = std::find_if(scenarios_.begin(), scenarios_.end(), [](const ScenarioDefinition& scenario) {
        return !scenario.sandboxLab;
    });
    return it != scenarios_.end() ? *it : scenarios_.front();
}
const std::vector<InterventionDefinition>& ContentRegistry::interventions() const { return interventions_; }
const std::vector<WorldActionDefinition>& ContentRegistry::worldActions() const { return worldActions_; }
const SimulationConfig& ContentRegistry::simulationConfig() const { return simulationConfig_; }
const std::vector<std::string>& ContentRegistry::loadErrors() const { return loadErrors_; }
bool ContentRegistry::loadedFromContent() const { return loadedFromContent_; }

} // namespace content
