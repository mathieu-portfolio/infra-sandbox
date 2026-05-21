#include "gameplay/world_actions/WorldActionDraftService.hpp"

#include "content/ContentRegistry.hpp"
#include "core/validation/ValueSpec.hpp"
#include "gameplay/world_actions/WorldActionPressureUtils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
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

bool unlocksMissingObservability(const UiState& state, const content::WorldActionDefinition& definition)
{
    for (const auto& unlock : definition.unlocksObservability) {
        if ((unlock == "metrics" && !state.observability.metricsUnlocked)
            || (unlock == "traffic" && !state.observability.trafficUnlocked)
            || ((unlock == "dependencies" || unlock == "tracing") && !state.observability.dependenciesUnlocked)
            || (unlock == "diagnostics" && !state.observability.diagnosticsUnlocked)) {
            return true;
        }
    }
    return false;
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

void WorldActionDraftService::generateDraft(UiState& state, ScenarioSession& session) const
{
    if (!state.worldActionDraft.empty()) {
        return;
    }

    std::vector<content::WorldActionDefinition> candidates = content::ContentRegistry::instance().worldActions();
    const PressureCategory dominant = session.simulation().pressure().dominantPressure;
    std::stable_sort(candidates.begin(), candidates.end(), [dominant, &state](const auto& lhs, const auto& rhs) {
        const bool lhsUnlock = unlocksMissingObservability(state, lhs);
        const bool rhsUnlock = unlocksMissingObservability(state, rhs);
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
        if (isCappedOutSpecialtyIncrease(rawCapacityBonus, effectiveCapacity, unlocksMissingObservability(state, definition))) {
            continue;
        }
        const PressureState pressureEffect = gameplay::world_actions::scaledPressureEffect(definition.pressureEffect, intensity);
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
            .pressurePreview = gameplay::world_actions::pressurePreviewText(pressureEffect),
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
