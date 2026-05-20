#pragma once

#include "gameplay/scenario/data/ScenarioEnums.hpp"

#include <algorithm>

struct EngineeringCapacity {
    int frontend = 1;
    int backend = 2;
    int infrastructure = 1;
    int data = 1;
    int total = 8;
};

struct EngineeringCost {
    EngineeringDomain domain = EngineeringDomain::Backend;
    int amount = 0;
};

inline int specialtyCapacityTotal(const EngineeringCapacity& capacity)
{
    return std::max(0, capacity.frontend)
        + std::max(0, capacity.backend)
        + std::max(0, capacity.infrastructure)
        + std::max(0, capacity.data);
}

inline bool hasPositiveSpecialtyBonus(const EngineeringCapacity& bonus)
{
    return bonus.frontend > 0
        || bonus.backend > 0
        || bonus.infrastructure > 0
        || bonus.data > 0;
}

inline bool hasNegativeSpecialtyBonus(const EngineeringCapacity& bonus)
{
    return bonus.frontend < 0
        || bonus.backend < 0
        || bonus.infrastructure < 0
        || bonus.data < 0;
}

inline EngineeringCapacity applyEngineeringCapacityBudgetCap(EngineeringCapacity base, const EngineeringCapacity& bonus)
{
    EngineeringCapacity result = base;
    result.total = std::max(0, base.total + bonus.total);

    auto applyReduction = [](int value, int delta) {
        return std::max(0, value + std::min(0, delta));
    };

    result.frontend = applyReduction(base.frontend, bonus.frontend);
    result.backend = applyReduction(base.backend, bonus.backend);
    result.infrastructure = applyReduction(base.infrastructure, bonus.infrastructure);
    result.data = applyReduction(base.data, bonus.data);

    int remaining = std::max(0, result.total - specialtyCapacityTotal(result));
    auto applyIncrease = [&remaining](int value, int delta) {
        if (delta <= 0 || remaining <= 0) {
            return value;
        }
        const int accepted = std::min(delta, remaining);
        remaining -= accepted;
        return value + accepted;
    };

    result.frontend = applyIncrease(result.frontend, bonus.frontend);
    result.backend = applyIncrease(result.backend, bonus.backend);
    result.infrastructure = applyIncrease(result.infrastructure, bonus.infrastructure);
    result.data = applyIncrease(result.data, bonus.data);
    return result;
}

inline EngineeringCapacity effectiveEngineeringCapacityBonus(const EngineeringCapacity& base, const EngineeringCapacity& bonus)
{
    const EngineeringCapacity capped = applyEngineeringCapacityBudgetCap(base, bonus);
    return {
        .frontend = capped.frontend - base.frontend,
        .backend = capped.backend - base.backend,
        .infrastructure = capped.infrastructure - base.infrastructure,
        .data = capped.data - base.data,
        .total = capped.total - base.total,
    };
}
