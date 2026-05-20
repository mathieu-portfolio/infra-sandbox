#include "gameplay/WorldActionController.hpp"

#include "content/ContentRegistry.hpp"
#include "content/ValueSpec.hpp"
#include "simulation/Mechanics.hpp"
#include "simulation/TopologyMutation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {
std::string capacityUsageSummary(const UiState& state)
{
    std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> domainUsage{};
    int total = 0;
    for (const auto& planned : state.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            domainUsage[static_cast<std::size_t>(cost.domain)] += cost.amount;
            total += cost.amount;
        }
    }
    if (total == 0) {
        return "0/" + std::to_string(state.engineeringCapacity.total);
    }

    std::string summary = std::to_string(total) + "/" + std::to_string(state.engineeringCapacity.total) + " budget";
    for (std::size_t index = 0; index < domainUsage.size(); ++index) {
        if (domainUsage[index] <= 0) {
            continue;
        }
        const auto domain = static_cast<EngineeringDomain>(index);
        summary += ", ";
        summary += engineeringDomainName(domain);
        summary += " ";
        summary += std::to_string(domainUsage[index]);
    }
    return summary;
}

EngineeringCapacity scaledCapacityBonus(EngineeringCapacity bonus, double intensity)
{
    auto scale = [intensity](int value) {
        return static_cast<int>(std::round(static_cast<double>(value) * intensity));
    };
    bonus.frontend = scale(bonus.frontend);
    bonus.backend = scale(bonus.backend);
    bonus.infrastructure = scale(bonus.infrastructure);
    bonus.data = scale(bonus.data);
    bonus.operations = scale(bonus.operations);
    bonus.total = scale(bonus.total);
    return bonus;
}

EngineeringCapacity sampledCapacityBonus(const content::WorldActionDefinition& definition, std::uint32_t seed)
{
    auto sample = [&](const char* field, const NumericRange& range) {
        return content::sampleRangeInt(range, seed, definition.id + field);
    };
    return {
        .frontend = sample(".capacity.frontend", definition.frontendCapacityBonusRange),
        .backend = sample(".capacity.backend", definition.backendCapacityBonusRange),
        .infrastructure = sample(".capacity.infrastructure", definition.infrastructureCapacityBonusRange),
        .data = sample(".capacity.data", definition.dataCapacityBonusRange),
        .operations = sample(".capacity.operations", definition.operationsCapacityBonusRange),
        .total = sample(".capacity.total", definition.totalCapacityBonusRange),
    };
}


void applyObservabilityUnlocks(UiState& state, const std::vector<std::string>& unlocks)
{
    for (const auto& unlock : unlocks) {
        if (unlock == "metrics") {
            state.observability.metricsUnlocked = true;
        } else if (unlock == "traffic") {
            state.observability.trafficUnlocked = true;
        } else if (unlock == "dependencies" || unlock == "tracing") {
            state.observability.dependenciesUnlocked = true;
        } else if (unlock == "diagnostics") {
            state.observability.diagnosticsUnlocked = true;
        }
    }
}

MechanicType mechanicForMutation(TopologyMutationType mutationType)
{
    return mutationType == TopologyMutationType::AddCache ? MechanicType::AddCache
        : mutationType == TopologyMutationType::AddReadReplica ? MechanicType::AddReadReplica
        : mutationType == TopologyMutationType::AddQueue ? MechanicType::AddQueue
        : MechanicType::AddRegionalCache;
}
}

void WorldActionController::generateDraft(UiState& state, ScenarioSession& session) const
{
    if (!state.worldActionDraft.empty()) {
        return;
    }

    std::vector<content::WorldActionDefinition> candidates = content::ContentRegistry::instance().worldActions();
    const PressureCategory dominant = session.simulation().pressure().dominantPressure;
    auto unlocksMissingObservability = [&state](const content::WorldActionDefinition& definition) {
        for (const auto& unlock : definition.unlocksObservability) {
            if ((unlock == "metrics" && !state.observability.metricsUnlocked)
                || (unlock == "traffic" && !state.observability.trafficUnlocked)
                || ((unlock == "dependencies" || unlock == "tracing") && !state.observability.dependenciesUnlocked)
                || (unlock == "diagnostics" && !state.observability.diagnosticsUnlocked)) {
                return true;
            }
        }
        return false;
    };
    std::stable_sort(candidates.begin(), candidates.end(), [dominant, &unlocksMissingObservability](const auto& lhs, const auto& rhs) {
        const bool lhsUnlock = unlocksMissingObservability(lhs);
        const bool rhsUnlock = unlocksMissingObservability(rhs);
        if (lhsUnlock != rhsUnlock) {
            return lhsUnlock;
        }
        const bool lhsMatch = std::find(lhs.affectedPressures.begin(), lhs.affectedPressures.end(), dominant) != lhs.affectedPressures.end();
        const bool rhsMatch = std::find(rhs.affectedPressures.begin(), rhs.affectedPressures.end(), dominant) != rhs.affectedPressures.end();
        return lhsMatch && !rhsMatch;
    });

    const std::size_t maxDraft = std::min<std::size_t>(3, candidates.size());
    for (std::size_t i = 0; i < maxDraft; ++i) {
        const auto& definition = candidates[i];
        const std::uint32_t seed = session.scenarioManager().run().seed + static_cast<std::uint32_t>(session.scenarioManager().elapsedSeconds());
        const double intensity = content::sampleNumber(seed, definition.id, definition.minIntensity, definition.maxIntensity);
        state.worldActionDraft.push_back({
            .id = definition.id,
            .name = definition.displayName,
            .description = definition.description,
            .category = definition.categories.empty() ? "World" : definition.categories.front(),
            .usefulWhen = definition.usefulWhen,
            .tradeOff = definition.tradeoffs,
            .showUsageDetails = definition.showUsageDetails,
            .iconId = definition.iconId,
            .capacityBonus = scaledCapacityBonus(sampledCapacityBonus(definition, seed), intensity),
            .intensity = intensity,
            .pressureResistance = content::sampleRange(definition.pressureResistanceRange, seed, definition.id + ".pressure_resistance") * intensity,
            .eventIntensityMultiplier = content::sampleRange(definition.eventIntensityMultiplierRange, seed, definition.id + ".event_intensity_multiplier"),
            .complexityDelta = content::sampleRange(definition.complexityDeltaRange, seed, definition.id + ".complexity_delta") * intensity,
            .durationSeconds = content::sampleRange(definition.durationSecondsRange, seed, definition.id + ".duration_turns"),
            .unlocksObservability = definition.unlocksObservability,
        });
    }
    state.selectedWorldActionIndex = -1;
    state.hoveredWorldActionIndex = -1;
    state.worldActionCapacityBonus = {};
    state.previewEngineeringCapacity = {};
    state.engineeringCapacityPreviewVisible = false;
    state.worldActionDraftVisible = state.eventPopupMode == EventPopupMode::None && !state.worldActionDraft.empty();
}

void WorldActionController::clearPlan(UiState& state) const
{
    state.eventPopupMode = EventPopupMode::None;
    state.eventPopupEvents.clear();
    state.eventPanelVisible = false;
    state.eventPanelAcknowledged = false;
    state.worldActionDraft.clear();
    state.worldActionDraftVisible = false;
    state.selectedWorldActionIndex = -1;
    state.hoveredWorldActionIndex = -1;
    state.worldActionCapacityBonus = {};
    state.previewEngineeringCapacity = {};
    state.engineeringCapacityPreviewVisible = false;
}

void WorldActionController::applyPlan(UiState& state, ScenarioSession& session) const
{
    if (state.selectedWorldActionIndex < 0 || state.selectedWorldActionIndex >= static_cast<int>(state.worldActionDraft.size())) {
        return;
    }
    const WorldActionDraft& action = state.worldActionDraft[static_cast<std::size_t>(state.selectedWorldActionIndex)];
    applyObservabilityUnlocks(state, action.unlocksObservability);
    session.scenarioManager().applyEngineeringCapacityBonus(action.capacityBonus);
    state.engineeringCapacity = session.scenarioManager().definition().engineeringCapacity;
    if (action.complexityDelta > 0.0) {
        session.simulation().addComplexity(action.complexityDelta);
    }
    state.actionHistory.push_back({session.simulation().timeSeconds(), action.name, "World", action.description, session.simulation().metrics(), false, true, 0.0});
    state.lastCapacityUsageSummary = state.lastCapacityUsageSummary.empty()
        ? "World action: " + action.name
        : state.lastCapacityUsageSummary + "; world action: " + action.name;

    // Once the world action is committed into ScenarioManager, it is no longer a preview.
    // Leaving the selection active let later planning/analysis frames add the same bonus
    // on top of the newly committed capacity.
    state.selectedWorldActionIndex = -1;
    state.hoveredWorldActionIndex = -1;
    state.worldActionCapacityBonus = {};
    state.previewEngineeringCapacity = {};
    state.engineeringCapacityPreviewVisible = false;
}

void WorldActionController::applyPlannedInterventions(UiState& state, ScenarioSession& session) const
{
    MechanicExecutor executor;
    TopologyBuilder topologyBuilder;
    const std::string worldActionSummary = state.lastCapacityUsageSummary;
    state.lastCapacityUsageSummary = capacityUsageSummary(state);
    if (!worldActionSummary.empty()) {
        state.lastCapacityUsageSummary = state.lastCapacityUsageSummary.empty()
            ? worldActionSummary
            : state.lastCapacityUsageSummary + "; " + worldActionSummary;
    }
    for (const auto& planned : state.plannedInterventions) {
        if (planned.kind == PlannedInterventionKind::Mechanic) {
            executor.execute(session.simulation(), planned.command);
            session.scenarioManager().notifyActionTriggered(planned.command.type);
            state.pendingVisualFeedbackEvents.push_back({
                .kind = planned.command.type == MechanicType::ThrottleTraffic ? VisualFeedbackKind::TrafficShift : VisualFeedbackKind::ActionAcknowledged,
                .targetNodeId = planned.command.targetId,
                .mechanic = planned.command.type,
                .label = planned.actionName,
            });
        } else {
            if (topologyBuilder.apply(session.simulation(), planned.mutation)) {
                const MechanicType mechanic = mechanicForMutation(planned.mutationType);
                session.scenarioManager().notifyActionTriggered(mechanic);
                state.pendingVisualFeedbackEvents.push_back({
                    .kind = VisualFeedbackKind::TopologyMutation,
                    .targetNodeId = -1,
                    .targetLinkId = -1,
                    .mechanic = mechanic,
                    .mutation = planned.mutationType,
                    .label = planned.actionName,
                });
            }
        }
        state.actionHistory.push_back({session.simulation().timeSeconds(), planned.actionName, planned.target, planned.preview, session.simulation().metrics(), true, false, 4.0});
    }
    state.plannedInterventions.clear();
    clearPlan(state);
}
