#pragma once

#include "simulation/metrics/Metrics.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

#include <array>
#include <cstddef>

namespace metrics_panel {

struct SpecializationSummary {
    MetricsSpecialization id = MetricsSpecialization::Frontend;
    const char* label = "";
    double health = 100.0;
    double trend = 0.0;
    bool implemented = false;
};

Color scoreColor(double score, bool higherIsBetter);

std::array<SpecializationSummary, static_cast<std::size_t>(MetricsSpecialization::Count)> specializationSummaries(
    const MetricsSnapshot& metrics,
    const UiState& state);

const SpecializationSummary& summaryFor(
    const std::array<SpecializationSummary, static_cast<std::size_t>(MetricsSpecialization::Count)>& summaries,
    MetricsSpecialization specialization);

const MetricContribution* strongestContribution(const MetricsSnapshot& metrics, MetricContributionDomain domain);

}
