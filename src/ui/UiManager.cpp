#include "ui/UiManager.hpp"

#include "ui/IconRegistry.hpp"

#include "raylib.h"

#include <string>

namespace {
EngineeringCapacity addCapacity(EngineeringCapacity base, const EngineeringCapacity& bonus)
{
    base.frontend += bonus.frontend;
    base.backend += bonus.backend;
    base.infrastructure += bonus.infrastructure;
    base.data += bonus.data;
    base.operations += bonus.operations;
    base.total += bonus.total;
    return base;
}
}

void UiManager::update(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused)
{
    UiContext context{&state_, GetScreenWidth(), GetScreenHeight(), paused};
    state_.sandboxMode = scenarioManager.definition().sandboxLab;
    state_.engineeringCapacity = addCapacity(scenarioManager.definition().engineeringCapacity, state_.worldActionCapacityBonus);
    updateActionObservations(simulation);
    updateMetricHistory(simulation);
    hudPanel_.update(context, simulation, scenarioManager);
    metricsPanel_.update(context, simulation);
    selectionPanel_.update(context, simulation);
    actionPanel_.update(context, simulation);
    timelinePanel_.update(context, simulation, scenarioManager);
    debugPanel_.update(context, simulation);
}

void UiManager::updateActionObservations(const Simulation& simulation)
{
    for (auto& entry : state_.actionHistory) {
        if (!entry.observationPending || entry.observationRecorded) {
            continue;
        }
        if (simulation.timeSeconds() - entry.timeSeconds < entry.observeAfterSeconds) {
            continue;
        }

        const auto& before = entry.beforeMetrics;
        const auto& after = simulation.metrics();
        if (entry.actionName.find("Scale") != std::string::npos) {
            if (after.apiQueueDepth < before.apiQueueDepth && after.databaseQueueDepth > before.databaseQueueDepth) {
                entry.message = "Queue pressure improved locally, while persistence pressure increased downstream.";
            } else if (after.apiQueueDepth < before.apiQueueDepth) {
                entry.message = "Queue pressure improved locally after scaling.";
            } else if (simulation.pressure().dominantPressure == PressureCategory::PersistencePressure || after.databaseQueueDepth >= before.databaseQueueDepth) {
                entry.message = "Scaling API had limited effect. Persistence pressure remains dominant.";
            }
        } else if (entry.actionName.find("Cache") != std::string::npos) {
            if (after.cacheHitRate > before.cacheHitRate || after.databaseQueueDepth < before.databaseQueueDepth) {
                entry.message = simulation.pressure().dominantPressure == PressureCategory::PersistencePressure
                    ? "Cache improved repeated reads, but persistence pressure still needs monitoring."
                    : "Cache reduced downstream persistence pressure.";
            }
        } else if (entry.actionName.find("Retries") != std::string::npos) {
            if (after.retryRatePerSecond < before.retryRatePerSecond) {
                entry.message = "Retry amplification decreased; validate whether visible failures changed.";
            } else if (simulation.pressure().dominantPressure == PressureCategory::RetryPressure) {
                entry.message = "Retry amplification remains visible near the overloaded service path.";
            }
        } else if (entry.actionName.find("Regional") != std::string::npos) {
            entry.message = "Traffic should localize near the deployed regional path if demand can use it.";
        } else if (entry.actionName.find("Replica") != std::string::npos) {
            entry.message = simulation.pressure().dominantPressure == PressureCategory::PersistencePressure
                ? "Replica deployed. Persistence pressure is still moving through the data tier."
                : "Read pressure is spreading across the persistence tier.";
        }
        state_.latestFeedback = entry.message;
        entry.observationRecorded = true;
        entry.observationPending = false;
    }
}

void UiManager::updateMetricHistory(const Simulation& simulation)
{
    const double now = simulation.timeSeconds();
    if (state_.lastMetricSampleTime >= 0.0 && now - state_.lastMetricSampleTime < 0.5) {
        return;
    }
    state_.lastMetricSampleTime = now;
    state_.metricsHistory.push_back(simulation.metrics());
    while (state_.metricsHistory.size() > 48) {
        state_.metricsHistory.pop_front();
    }
}

void UiManager::draw(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused) const
{
    UiState* mutableState = const_cast<UiState*>(&state_);
    UiContext context{mutableState, GetScreenWidth(), GetScreenHeight(), paused};
    metricsPanel_.draw(context, simulation);
    actionPanel_.draw(context, simulation);
    timelinePanel_.draw(context, simulation, scenarioManager);
    selectionPanel_.draw(context, simulation);
    debugPanel_.draw(context, simulation);
    hudPanel_.draw(context, simulation, scenarioManager);
    actionPanel_.drawWorldActionOverlay(context);
}

const UiState& UiManager::state() const
{
    return state_;
}

UiState& UiManager::state()
{
    return state_;
}

const OverlayController& UiManager::overlayController() const
{
    return overlayController_;
}

void UiManager::releaseResources()
{
    IconRegistry::instance().release();
}
