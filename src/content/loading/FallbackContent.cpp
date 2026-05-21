#include "content/ContentRegistry.hpp"

namespace content {

void ContentRegistry::loadFallbackContent()
{
    clearLoadedContent();
    loadedFromContent_ = false;
    progressionTiers_ = {{
        .id = "fallback",
        .displayName = "Fallback",
        .description = "Minimal fallback progression.",
        .tier = ProgressionTier::Foundations,
        .name = "Fallback",
        .visibleMetrics = {"Input rate", "Queue depth", "Latency"},
        .availableMechanics = {MechanicType::ScaleUp, MechanicType::ThrottleTraffic},
        .allowedPressures = {PressureCategory::TrafficPressure, PressureCategory::QueuePressure},
        .allowedNodeTypes = {NodeType::ClientCluster, NodeType::ApiService},
    }};
    ScenarioDefinition scenario;
    scenario.id = "fallback_scenario";
    scenario.displayName = "Fallback Scenario";
    scenario.name = scenario.displayName;
    scenario.description = "Content failed to load; this tiny scenario keeps the simulation usable.";
    scenario.minimumTier = ProgressionTier::Foundations;
    scenario.allowedMechanics = {MechanicType::ScaleUp, MechanicType::ThrottleTraffic};
    scenario.trafficProfile = {.id = "fallback_constant", .displayName = "Constant", .name = "Constant", .type = TrafficProfileType::Constant, .baseMultiplier = 1.0};
    scenario.nodes = {
        {.id = "clients", .name = "Clients", .type = NodeType::ClientCluster, .requestRatePerSecond = 2.0},
        {.id = "api", .name = "API", .type = NodeType::ApiService, .processingCapacityPerSecond = 4.0},
    };
    scenario.links = {{.sourceNode = 0, .targetNode = 1, .baseLatencySeconds = 0.3, .bandwidthPerSecond = 100.0}};
    scenario.objectives = {{.id = "fallback_survive", .displayName = "Survive", .type = ScenarioObjectiveType::SurviveDuration, .summary = "Keep fallback service running for 3 turns.", .durationTurns = 3}};
    scenarios_ = {std::move(scenario)};
    interventions_ = {
        {
            .id = "scale_up",
            .displayName = "Scale Up",
            .description = "Increase API service capacity.",
            .expectedBenefits = "Processing capacity, queue pressure",
            .tradeoffs = "May not solve downstream bottlenecks.",
            .positiveEffects = {"API queue pressure decreases", "Compute headroom increases"},
            .negativeEffects = {"Downstream persistence pressure can become dominant", "Operational complexity increases"},
            .pressureShifts = {"Queue pressure can shift toward persistence"},
            .engineeringCosts = {{EngineeringDomain::Infrastructure, 1}},
            .mechanic = MechanicType::ScaleUp,
            .complexityCost = 1.0,
            .maxScaleLevel = 3,
            .diminishingReturn = 0.72,
            .pressureEffect = {.backend = {.queuePressure = -0.04, .computeIntensity = -0.06, .serviceFragmentation = 0.02}},
        },
    };
    worldActions_ = {
        {
            .id = "fallback_tooling",
            .displayName = "Improve Backend Tooling",
            .description = "Free a small amount of backend capacity for the next plan.",
            .categories = {"Organization"},
            .usefulWhen = "Backend work is constraining local actions.",
            .tradeoffs = "Creates little immediate infrastructure change.",
            .iconId = "action.generic",
            .capacityBonus = {.backend = 1},
            .backendCapacityBonusRange = {1.0, 1.0},
            .pressureEffect = {.backend = {.serviceFragmentation = -0.04}},
        },
    };
}


} // namespace content
