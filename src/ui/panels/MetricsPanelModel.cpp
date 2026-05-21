#include "ui/panels/MetricsPanelModel.hpp"

#include <algorithm>
#include <cmath>

namespace metrics_panel {
namespace {

double clampScore(double value)
{
    return std::clamp(value, 0.0, 100.0);
}

double frontendHealth(const MetricsSnapshot& metrics)
{
    return clampScore(
        100.0
        - metrics.frontend.perceivedLatency * 26.0
        - metrics.frontend.framePressure * 0.18
        - metrics.frontend.sessionStalenessRisk * 0.35
        + metrics.frontend.sessionWarmth * 0.08);
}

double backendHealth(const MetricsSnapshot& metrics)
{
    return clampScore(100.0 - std::max({metrics.backend.requestLoad, metrics.backend.queuePressure, metrics.backend.computeIntensity, metrics.backend.reliabilityRisk}));
}

double networkHealth(const MetricsSnapshot& metrics)
{
    return clampScore(100.0 - metrics.network.deliveryPressure);
}

double databaseHealth(const MetricsSnapshot& metrics)
{
    return clampScore(100.0 - metrics.database.persistenceRisk);
}

double runtimeHealth(const MetricsSnapshot& metrics)
{
    return clampScore(100.0 - metrics.runtime.executionRisk);
}

const MetricsSnapshot* comparisonSnapshot(const UiState& state)
{
    if (state.metricsHistory.size() < 2) {
        return nullptr;
    }
    return &state.metricsHistory.front();
}

}

Color scoreColor(double score, bool higherIsBetter)
{
    const double normalized = higherIsBetter ? score : 100.0 - score;
    if (normalized >= 75.0) {
        return {86, 210, 151, 255};
    }
    if (normalized >= 45.0) {
        return {245, 184, 76, 255};
    }
    return {235, 86, 100, 255};
}

std::array<SpecializationSummary, static_cast<std::size_t>(MetricsSpecialization::Count)> specializationSummaries(
    const MetricsSnapshot& metrics,
    const UiState& state)
{
    double frontendTrend = 0.0;
    double backendTrend = 0.0;
    double networkTrend = 0.0;
    double databaseTrend = 0.0;
    double runtimeTrend = 0.0;
    if (const MetricsSnapshot* before = comparisonSnapshot(state); before != nullptr) {
        frontendTrend = frontendHealth(metrics) - frontendHealth(*before);
        backendTrend = backendHealth(metrics) - backendHealth(*before);
        networkTrend = networkHealth(metrics) - networkHealth(*before);
        databaseTrend = databaseHealth(metrics) - databaseHealth(*before);
        runtimeTrend = runtimeHealth(metrics) - runtimeHealth(*before);
    }
    return {
        SpecializationSummary{MetricsSpecialization::Frontend, "Frontend", frontendHealth(metrics), frontendTrend, true},
        SpecializationSummary{MetricsSpecialization::Backend, "Backend", backendHealth(metrics), backendTrend, true},
        SpecializationSummary{MetricsSpecialization::Network, "Network", networkHealth(metrics), networkTrend, true},
        SpecializationSummary{MetricsSpecialization::Database, "Database", databaseHealth(metrics), databaseTrend, true},
        SpecializationSummary{MetricsSpecialization::Runtime, "Runtime", runtimeHealth(metrics), runtimeTrend, true},
    };
}

const SpecializationSummary& summaryFor(
    const std::array<SpecializationSummary, static_cast<std::size_t>(MetricsSpecialization::Count)>& summaries,
    MetricsSpecialization specialization)
{
    return summaries[static_cast<std::size_t>(specialization)];
}

const MetricContribution* strongestContribution(const MetricsSnapshot& metrics, MetricContributionDomain domain)
{
    const MetricContribution* strongest = nullptr;
    for (const auto& contribution : metrics.contributions) {
        if (contribution.domain != domain) {
            continue;
        }
        if (strongest == nullptr || std::abs(contribution.amount) > std::abs(strongest->amount)) {
            strongest = &contribution;
        }
    }
    return strongest;
}

}
