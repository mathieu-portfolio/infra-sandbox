#include "gameplay/WorldActionController.hpp"

#include "content/ContentRegistry.hpp"
#include "core/validation/ValueSpec.hpp"
#include "core/simulation/Mechanics.hpp"
#include "simulation/topology/TopologyMutation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
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
        .total = sample(".capacity.total", definition.totalCapacityBonusRange),
    };
}

PressureState scaledPressureEffect(PressureState effect, double intensity)
{
    auto scale = [intensity](double value) {
        return value * intensity;
    };
    effect.frontend.assetWeight = scale(effect.frontend.assetWeight);
    effect.frontend.renderComplexity = scale(effect.frontend.renderComplexity);
    effect.frontend.cacheEfficiency = scale(effect.frontend.cacheEfficiency);
    effect.frontend.realtimeIntensity = scale(effect.frontend.realtimeIntensity);
    effect.frontend.sessionPersistence = scale(effect.frontend.sessionPersistence);
    effect.frontend.mobileCompatibility = scale(effect.frontend.mobileCompatibility);
    effect.backend.requestLoad = scale(effect.backend.requestLoad);
    effect.backend.queuePressure = scale(effect.backend.queuePressure);
    effect.backend.computeIntensity = scale(effect.backend.computeIntensity);
    effect.backend.serviceFragmentation = scale(effect.backend.serviceFragmentation);
    effect.network.bandwidthPressure = scale(effect.network.bandwidthPressure);
    effect.network.latencySensitivity = scale(effect.network.latencySensitivity);
    effect.network.trafficBurstiness = scale(effect.network.trafficBurstiness);
    return effect;
}

bool hasPressureEffect(const PressureState& effect)
{
    return effect.frontend.assetWeight != 0.0
        || effect.frontend.renderComplexity != 0.0
        || effect.frontend.cacheEfficiency != 0.0
        || effect.frontend.realtimeIntensity != 0.0
        || effect.frontend.sessionPersistence != 0.0
        || effect.frontend.mobileCompatibility != 0.0
        || effect.backend.requestLoad != 0.0
        || effect.backend.queuePressure != 0.0
        || effect.backend.computeIntensity != 0.0
        || effect.backend.serviceFragmentation != 0.0
        || effect.network.bandwidthPressure != 0.0
        || effect.network.latencySensitivity != 0.0
        || effect.network.trafficBurstiness != 0.0;
}

std::string signedPressurePart(const char* label, double value, bool lowerIsBetter)
{
    if (std::abs(value) < 0.001) {
        return {};
    }
    const bool beneficial = lowerIsBetter ? value < 0.0 : value > 0.0;
    std::string text = beneficial ? "eases " : "raises ";
    text += label;
    return text;
}

std::string pressurePreviewText(const PressureState& effect)
{
    std::vector<std::string> parts;
    auto add = [&parts](std::string part) {
        if (!part.empty()) {
            parts.push_back(std::move(part));
        }
    };
    add(signedPressurePart("asset weight", effect.frontend.assetWeight, true));
    add(signedPressurePart("render complexity", effect.frontend.renderComplexity, true));
    add(signedPressurePart("cache efficiency", effect.frontend.cacheEfficiency, false));
    add(signedPressurePart("realtime intensity", effect.frontend.realtimeIntensity, true));
    add(signedPressurePart("session persistence", effect.frontend.sessionPersistence, false));
    add(signedPressurePart("backend load", effect.backend.requestLoad, true));
    add(signedPressurePart("queue pressure", effect.backend.queuePressure, true));
    add(signedPressurePart("service fragmentation", effect.backend.serviceFragmentation, true));
    add(signedPressurePart("bandwidth pressure", effect.network.bandwidthPressure, true));
    add(signedPressurePart("latency sensitivity", effect.network.latencySensitivity, true));
    add(signedPressurePart("burstiness", effect.network.trafficBurstiness, true));
    if (parts.empty()) {
        return {};
    }
    std::string text = "Pressure effect: " + parts.front();
    for (std::size_t i = 1; i < parts.size() && i < 3; ++i) {
        text += ", ";
        text += parts[i];
    }
    return text;
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

bool hasAnyCapacityDelta(const EngineeringCapacity& bonus)
{
    return bonus.frontend != 0
        || bonus.backend != 0
        || bonus.infrastructure != 0
        || bonus.data != 0
        || bonus.total != 0;
}

bool isCappedOutSpecialtyIncrease(
    const EngineeringCapacity& rawBonus,
    const EngineeringCapacity& effectiveBonus,
    bool unlocksMissingObservability)
{
    if (unlocksMissingObservability) {
        return false;
    }
    return hasPositiveSpecialtyBonus(rawBonus)
        && !hasNegativeSpecialtyBonus(rawBonus)
        && rawBonus.total <= 0
        && !hasAnyCapacityDelta(effectiveBonus);
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

    constexpr std::size_t maxDraft = 3;
    for (const auto& definition : candidates) {
        if (state.worldActionDraft.size() >= maxDraft) {
            break;
        }
        const std::uint32_t seed = session.scenarioManager().run().seed + static_cast<std::uint32_t>(session.scenarioManager().elapsedSeconds());
        const double intensity = content::sampleNumber(seed, definition.id, definition.minIntensity, definition.maxIntensity);
        const EngineeringCapacity rawCapacityBonus = scaledCapacityBonus(sampledCapacityBonus(definition, seed), intensity);
        const EngineeringCapacity effectiveCapacity = effectiveEngineeringCapacityBonus(
            session.scenarioManager().definition().engineeringCapacity,
            rawCapacityBonus);
        if (isCappedOutSpecialtyIncrease(rawCapacityBonus, effectiveCapacity, unlocksMissingObservability(definition))) {
            continue;
        }
        const PressureState pressureEffect = scaledPressureEffect(definition.pressureEffect, intensity);
        state.worldActionDraft.push_back({
            .id = definition.id,
            .name = definition.displayName,
            .description = definition.description,
            .category = definition.categories.empty() ? "World" : definition.categories.front(),
            .usefulWhen = definition.usefulWhen,
            .tradeOff = definition.tradeoffs,
            .showUsageDetails = definition.showUsageDetails,
            .iconId = definition.iconId,
            .capacityBonus = effectiveCapacity,
            .intensity = intensity,
            .pressureResistance = content::sampleRange(definition.pressureResistanceRange, seed, definition.id + ".pressure_resistance") * intensity,
            .eventIntensityMultiplier = content::sampleRange(definition.eventIntensityMultiplierRange, seed, definition.id + ".event_intensity_multiplier"),
            .complexityDelta = content::sampleRange(definition.complexityDeltaRange, seed, definition.id + ".complexity_delta") * intensity,
            .durationSeconds = content::sampleRange(definition.durationSecondsRange, seed, definition.id + ".duration_turns"),
            .pressureEffect = pressureEffect,
            .pressurePreview = pressurePreviewText(pressureEffect),
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
    if (hasPressureEffect(action.pressureEffect)) {
        session.simulation().applyPressureEffect(action.pressureEffect);
    }
    if (action.complexityDelta > 0.0) {
        // TODO: Keep this direct complexity path only for legacy content that
        // has not yet expressed its trade-off as a backend/service pressure.
        session.simulation().addComplexity(action.complexityDelta);
    }
    state.actionHistory.push_back({
        session.simulation().timeSeconds(),
        action.id,
        action.name,
        "World",
        action.description,
        session.simulation().metrics(),
        false,
        true,
        0.0,
    });
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
        state.actionHistory.push_back({
            session.simulation().timeSeconds(),
            planned.actionId,
            planned.actionName,
            planned.target,
            planned.preview,
            session.simulation().metrics(),
            true,
            false,
            4.0,
        });
    }
    state.plannedInterventions.clear();
    clearPlan(state);
}
