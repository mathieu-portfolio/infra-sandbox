#include "content/ContentRegistry.hpp"

#include <algorithm>
#include <set>

namespace content {
namespace {

void validateRange(const NumericRange& range, const std::string& label, ContentLoadResult& result)
{
    content::validateRange(range, label, result.errors);
}

void validateBurstRanges(const BurstScenario& burst, const std::string& label, ContentLoadResult& result)
{
    validateRange(burst.multiplierRange, label + " burst multiplier", result);
    validateRange(burst.periodSecondsRange, label + " burst period", result);
    validateRange(burst.durationSecondsRange, label + " burst duration", result);
}

void validateEventRanges(const EventDefinition& event, const std::string& label, ContentLoadResult& result)
{
    validateRange(event.trigger.timeSecondsRange, label + " trigger time_seconds", result);
    validateRange(event.trigger.delaySecondsRange, label + " trigger delay_seconds", result);
    validateRange(event.effect.trafficMultiplierRange, label + " traffic_multiplier", result);
    validateRange(event.effect.burstMultiplierRange, label + " burst_multiplier", result);
    validateRange(event.effect.databaseCapacityMultiplierRange, label + " database_capacity_multiplier", result);
    validateRange(event.effect.latencyMultiplierRange, label + " latency_multiplier", result);
    validateRange(event.effect.retryDelayMultiplierRange, label + " retry_delay_multiplier", result);
    if (event.effect.databaseHeavyShareRange) {
        validateRange(*event.effect.databaseHeavyShareRange, label + " database_heavy_share", result);
    }
    validateRange(event.durationSecondsRange, label + " duration_seconds", result);
    validateRange(event.intensityRange, label + " intensity", result);
}

void requireIdSet(const std::string& domain, const std::vector<std::string>& ids, ContentLoadResult& result)
{
    std::set<std::string> seen;
    for (const auto& id : ids) {
        if (id.empty()) {
            result.errors.push_back(domain + " has empty id.");
        }
        if (!seen.insert(id).second) {
            result.errors.push_back(domain + " has duplicate id: " + id);
        }
    }
}

} // namespace

void ContentRegistry::validate(ContentLoadResult& result) const
{
    const auto& pressure = simulationConfig_.pressureAnalysis;
    if (pressure.queueCapacityWindow <= 0.0 || pressure.timeoutRateScale <= 0.0 || pressure.retryRateScale <= 0.0) {
        result.errors.push_back("Pressure analysis tuning has invalid non-positive scale values.");
    }
    if (pressure.pressureHistoryLimit == 0 || pressure.recentEventLimit == 0) {
        result.errors.push_back("Pressure analysis tuning has invalid history limits.");
    }
    if (pressure.recurringPressureSampleCount <= 0 || pressure.recurringPressureMinimum <= 0) {
        result.errors.push_back("Pressure analysis tuning has invalid recurring pressure settings.");
    }

    std::vector<std::string> scenarioIds;
    for (const auto& scenario : scenarios_) {
        scenarioIds.push_back(scenario.id);
        if (scenario.nodes.empty()) result.errors.push_back("Scenario " + scenario.id + " has no topology nodes.");
        if (scenario.links.empty()) result.errors.push_back("Scenario " + scenario.id + " has no topology links.");
        if (scenario.objectives.empty()) result.errors.push_back("Scenario " + scenario.id + " has no objectives.");
        if (scenario.trafficProfile.baseMultiplier < 0.0) result.errors.push_back("Scenario " + scenario.id + " has invalid traffic multiplier.");
        validateRange(scenario.trafficProfile.baseMultiplierRange, "Scenario " + scenario.id + " traffic base_multiplier", result);
        validateRange(scenario.trafficProfile.growthPerSecondRange, "Scenario " + scenario.id + " traffic growth_per_turn", result);
        const auto& evolution = scenario.trafficProfile.evolution;
        if (evolution.pressureSensitivity < 0.0
            || evolution.churnSensitivity < 0.0
            || evolution.migrationSensitivity < 0.0
            || evolution.reroutePressureSensitivity < 0.0
            || evolution.rerouteLatencySensitivity < 0.0
            || evolution.dynamicRetrySensitivity < 0.0
            || evolution.burstAmplification < 0.0) {
            result.errors.push_back("Scenario " + scenario.id + " has invalid negative traffic evolution value.");
        }
        validateBurstRanges(scenario.bursts, "Scenario " + scenario.id, result);
        if (scenario.turnDuration.value <= 0.0 || scenario.turnDuration.simulationSeconds <= 0.0) {
            result.errors.push_back("Scenario " + scenario.id + " has invalid turn duration.");
        }
        const EngineeringCapacity& capacity = scenario.engineeringCapacity;
        if (capacity.frontend < 0 || capacity.backend < 0 || capacity.infrastructure < 0 || capacity.data < 0 || capacity.total < 0) {
            result.errors.push_back("Scenario " + scenario.id + " has invalid engineering capacity.");
        }
        for (const auto& node : scenario.nodes) {
            if (node.requestRatePerSecond < 0.0 || node.processingCapacityPerSecond < 0.0) {
                result.errors.push_back("Scenario " + scenario.id + " has invalid node numeric ranges.");
            }
        }
        for (const auto& phase : scenario.phases) {
            validateRange(phase.startTimeSecondsRange, "Scenario " + scenario.id + " phase " + phase.name + " start_time_seconds", result);
            validateRange(phase.durationSecondsRange, "Scenario " + scenario.id + " phase " + phase.name + " duration_turns", result);
            validateRange(phase.trafficMultiplierRange, "Scenario " + scenario.id + " phase " + phase.name + " traffic_multiplier", result);
            if (phase.burstOverride) {
                validateBurstRanges(*phase.burstOverride, "Scenario " + scenario.id + " phase " + phase.name, result);
            }
            if (phase.transitionDuration.value <= 0.0 || phase.transitionDuration.simulationSeconds <= 0.0) {
                result.errors.push_back("Scenario " + scenario.id + " phase " + phase.name + " has invalid transition duration.");
            }
        }
        for (const auto& event : scenario.events) {
            validateEventRanges(event, "Scenario " + scenario.id + " event " + event.id, result);
        }
        for (const auto& event : scenario.sandboxEvents) {
            validateEventRanges(event, "Scenario " + scenario.id + " sandbox event " + event.id, result);
        }
        for (const auto& modifier : scenario.optionalModifiers) {
            validateRange(modifier.selectionWeightRange, "Scenario " + scenario.id + " modifier " + modifier.id + " selection_weight", result);
            validateRange(modifier.trafficMultiplierRange, "Scenario " + scenario.id + " modifier " + modifier.id + " traffic_multiplier", result);
        }
    }
    requireIdSet("Scenario", scenarioIds, result);
    if (!packMetadata_.defaultScenarioId.empty() && std::find(scenarioIds.begin(), scenarioIds.end(), packMetadata_.defaultScenarioId) == scenarioIds.end()) {
        result.errors.push_back("Content pack " + packMetadata_.id + " references missing default scenario: " + packMetadata_.defaultScenarioId);
    }

    std::vector<std::string> tierIds;
    for (const auto& tier : progressionTiers_) tierIds.push_back(tier.id);
    requireIdSet("Progression", tierIds, result);

    std::vector<std::string> interventionIds;
    for (const auto& intervention : interventions_) interventionIds.push_back(intervention.id);
    requireIdSet("Intervention", interventionIds, result);

    std::vector<std::string> worldActionIds;
    for (const auto& action : worldActions_) worldActionIds.push_back(action.id);
    requireIdSet("World action", worldActionIds, result);
}


} // namespace content
