#include "gameplay/Scenario.hpp"

#include "content/ContentPackManager.hpp"
#include "content/ContentRegistry.hpp"

#include <algorithm>

namespace {
void ensureContentLoaded()
{
    if (!content::ContentRegistry::instance().scenarios().empty()) {
        return;
    }

    content::ContentPackManager packManager;
    if (packManager.discoverDefaultLocations().loaded && packManager.loadPack("vanilla").loaded) {
        return;
    }

    content::ContentRegistry::instance().loadFallbackContent();
}
}

const std::vector<ProgressionTierDefinition>& ProgressionRegistry::definitions()
{
    ensureContentLoaded();
    return content::ContentRegistry::instance().progressionTiers();
}

const ProgressionTierDefinition& ProgressionRegistry::definition(ProgressionTier tier)
{
    ensureContentLoaded();
    return content::ContentRegistry::instance().progressionTier(tier);
}

ScenarioDefinition Scenario::createDefault()
{
    ensureContentLoaded();
    return content::ContentRegistry::instance().defaultScenario();
}

std::vector<ScenarioDefinition> ScenarioRegistry::createAll()
{
    ensureContentLoaded();
    return content::ContentRegistry::instance().scenarios();
}

ScenarioDefinition ScenarioRegistry::singleServiceOverload()
{
    const auto scenarios = createAll();
    const auto it = std::find_if(scenarios.begin(), scenarios.end(), [](const ScenarioDefinition& scenario) {
        return scenario.id == "local_startup" || scenario.archetype == ScenarioArchetype::LocalStartup;
    });
    return it != scenarios.end() ? *it : Scenario::createDefault();
}

ScenarioDefinition ScenarioRegistry::databaseBottleneck()
{
    const auto scenarios = createAll();
    const auto it = std::find_if(scenarios.begin(), scenarios.end(), [](const ScenarioDefinition& scenario) {
        return scenario.id == "database_bottleneck" || scenario.archetype == ScenarioArchetype::DatabasePressure;
    });
    return it != scenarios.end() ? *it : Scenario::createDefault();
}

ScenarioDefinition ScenarioRegistry::burstTraffic()
{
    const auto scenarios = createAll();
    const auto it = std::find_if(scenarios.begin(), scenarios.end(), [](const ScenarioDefinition& scenario) {
        return scenario.id == "burst_traffic" || scenario.archetype == ScenarioArchetype::BurstTraffic;
    });
    return it != scenarios.end() ? *it : Scenario::createDefault();
}

const char* progressionTierName(ProgressionTier tier)
{
    switch (tier) {
    case ProgressionTier::Foundations:
        return "Foundations";
    case ProgressionTier::LocalScale:
        return "Local Scale";
    case ProgressionTier::StateAndCache:
        return "State and Cache";
    case ProgressionTier::FailureFeedback:
        return "Failure Feedback";
    case ProgressionTier::GeographicScale:
        return "Geographic Scale";
    case ProgressionTier::DistributedSystems:
        return "Distributed Systems";
    case ProgressionTier::Complexity:
        return "Complexity";
    }
    return "Unknown";
}

const char* scenarioArchetypeName(ScenarioArchetype archetype)
{
    switch (archetype) {
    case ScenarioArchetype::FirstRequest:
        return "First Request";
    case ScenarioArchetype::LocalStartup:
        return "Local Startup";
    case ScenarioArchetype::DatabasePressure:
        return "Database Pressure";
    case ScenarioArchetype::BurstTraffic:
        return "Burst Traffic";
    case ScenarioArchetype::TransatlanticLatency:
        return "Transatlantic Latency";
    case ScenarioArchetype::GlobalReadPlatform:
        return "Global Read Platform";
    }
    return "Unknown";
}

const char* engineeringDomainName(EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend:
        return "Frontend";
    case EngineeringDomain::Backend:
        return "Backend";
    case EngineeringDomain::Infrastructure:
        return "Infrastructure";
    case EngineeringDomain::Data:
        return "Data";
    case EngineeringDomain::Operations:
        return "Operations";
    case EngineeringDomain::Count:
        break;
    }
    return "Unknown";
}

const char* gameplayDurationUnitName(GameplayDurationUnit unit)
{
    switch (unit) {
    case GameplayDurationUnit::Turns:
        return "turns";
    case GameplayDurationUnit::Seconds:
        return "seconds";
    case GameplayDurationUnit::Minutes:
        return "minutes";
    case GameplayDurationUnit::Days:
        return "days";
    case GameplayDurationUnit::Months:
        return "months";
    case GameplayDurationUnit::Years:
        return "years";
    }
    return "time";
}

double gameplayDurationCalendarDays(const GameplayDuration& duration)
{
    if (!duration.advancesCalendar) {
        return 0.0;
    }
    switch (duration.unit) {
    case GameplayDurationUnit::Turns:
        return 0.0;
    case GameplayDurationUnit::Seconds:
        return duration.value / 86400.0;
    case GameplayDurationUnit::Minutes:
        return duration.value / 1440.0;
    case GameplayDurationUnit::Days:
        return duration.value;
    case GameplayDurationUnit::Months:
        return duration.value * 30.0;
    case GameplayDurationUnit::Years:
        return duration.value * 360.0;
    }
    return 0.0;
}
