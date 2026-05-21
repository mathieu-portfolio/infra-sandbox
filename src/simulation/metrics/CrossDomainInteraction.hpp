#pragma once

#include "simulation/metrics/Metrics.hpp"

class CrossDomainInteraction {
public:
    [[nodiscard]] static PressureState apply(const PressureState& state);
};
