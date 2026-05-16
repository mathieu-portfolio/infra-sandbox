#include "ui/UiManager.hpp"

#include "ui/IconRegistry.hpp"

#include "raylib.h"

void UiManager::update(const Simulation& simulation, const ScenarioManager&, bool paused)
{
    UiContext context{&state_, GetScreenWidth(), GetScreenHeight(), paused};
    updateActionObservations(simulation);
    updateMetricHistory(simulation);
    hudPanel_.update(context, simulation);
    metricsPanel_.update(context, simulation);
    selectionPanel_.update(context, simulation);
    interventionPanel_.update(context, simulation);
    timelinePanel_.update(context, simulation);
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
            if (after.apiQueueDepth < before.apiQueueDepth) {
                entry.message = "API queue dropped after scaling.";
            } else if (after.databaseQueueDepth >= before.databaseQueueDepth) {
                entry.message = "Scaling API had limited effect. DB queue remains high.";
            }
        } else if (entry.actionName.find("Cache") != std::string::npos) {
            if (after.cacheHitRate > before.cacheHitRate || after.databaseQueueDepth < before.databaseQueueDepth) {
                entry.message = "Cache behavior improved repeated reads or DB pressure.";
            }
        } else if (entry.actionName.find("Retries") != std::string::npos) {
            if (after.retryRatePerSecond < before.retryRatePerSecond) {
                entry.message = "Retry traffic decreased after policy change.";
            }
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
    hudPanel_.draw(context, simulation, scenarioManager);
    interventionPanel_.draw(context, simulation);
    timelinePanel_.draw(context, simulation, scenarioManager);
    selectionPanel_.draw(context, simulation);
    debugPanel_.draw(context, simulation);
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
