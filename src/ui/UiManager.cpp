#include "ui/UiManager.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/DockLayout.hpp"
#include "ui/viewmodels/UiFrameView.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace {

Rectangle toRaylib(UiRect rect)
{
    return {rect.x, rect.y, rect.width, rect.height};
}

float centerY(UiRect rect)
{
    return rect.y + rect.height * 0.5f;
}

void drawDockResizeHandles(const DockLayoutFrame& frame, const DockLayoutState& state)
{
    const DockResizeHandle visible = state.activeHandle != DockResizeHandle::None ? state.activeHandle : state.hoveredHandle;
    if (visible == DockResizeHandle::None) {
        return;
    }

    const Color fill = state.activeHandle != DockResizeHandle::None
        ? Color{89, 196, 255, 120}
        : Color{89, 196, 255, 70};
    const Color line = state.activeHandle != DockResizeHandle::None
        ? Color{125, 211, 252, 220}
        : Color{125, 211, 252, 150};

    if (visible == DockResizeHandle::LeftRightEdge) {
        DrawRectangleRec(toRaylib(frame.leftResizeHandle), fill);
        DrawLineV({frame.leftResizeHandle.x + frame.leftResizeHandle.width * 0.5f, frame.leftResizeHandle.y + 12.0f},
                  {frame.leftResizeHandle.x + frame.leftResizeHandle.width * 0.5f, frame.leftResizeHandle.y + frame.leftResizeHandle.height - 12.0f}, line);
    } else if (visible == DockResizeHandle::RightLeftEdge) {
        DrawRectangleRec(toRaylib(frame.rightResizeHandle), fill);
        DrawLineV({frame.rightResizeHandle.x + frame.rightResizeHandle.width * 0.5f, frame.rightResizeHandle.y + 12.0f},
                  {frame.rightResizeHandle.x + frame.rightResizeHandle.width * 0.5f, frame.rightResizeHandle.y + frame.rightResizeHandle.height - 12.0f}, line);
    } else if (visible == DockResizeHandle::BottomTopEdge) {
        DrawRectangleRec(toRaylib(frame.bottomResizeHandle), fill);
        DrawLineV({frame.bottomResizeHandle.x + 12.0f, centerY(frame.bottomResizeHandle)},
                  {frame.bottomResizeHandle.x + frame.bottomResizeHandle.width - 12.0f, centerY(frame.bottomResizeHandle)}, line);
    }
}

EngineeringCapacity displayCapacityWithBonus(EngineeringCapacity base, const EngineeringCapacity& bonus)
{
    return applyEngineeringCapacityBudgetCap(base, bonus);
}

bool hasActiveCapacityPreview(const UiState& state)
{
    if (state.gameplayPhase != GameplayPhase::Planning) {
        return false;
    }
    return (state.hoveredWorldActionIndex >= 0
            && state.hoveredWorldActionIndex < static_cast<int>(state.worldActionDraft.size()))
        || (state.selectedWorldActionIndex >= 0
            && state.selectedWorldActionIndex < static_cast<int>(state.worldActionDraft.size()));
}

EngineeringCapacity activeCapacityBonus(const UiState& state)
{
    if (state.gameplayPhase != GameplayPhase::Planning) {
        return {};
    }
    if (state.hoveredWorldActionIndex >= 0
        && state.hoveredWorldActionIndex < static_cast<int>(state.worldActionDraft.size())) {
        return state.worldActionDraft[static_cast<std::size_t>(state.hoveredWorldActionIndex)].capacityBonus;
    }
    if (state.selectedWorldActionIndex >= 0
        && state.selectedWorldActionIndex < static_cast<int>(state.worldActionDraft.size())) {
        return state.worldActionDraft[static_cast<std::size_t>(state.selectedWorldActionIndex)].capacityBonus;
    }
    return {};
}
}

void UiManager::update(const Simulation& simulation, const ScenarioManager& scenarioManager, const content::ContentPackManager& packManager, bool paused)
{
    UiContext context{&state_, GetScreenWidth(), GetScreenHeight(), paused};
    state_.sandboxMode = scenarioManager.definition().sandboxLab;
    // Keep committed capacity and temporary preview capacity separate.
    // The old code wrote the preview directly into engineeringCapacity, which made
    // the panel and capacity checks read different values across phase changes.
    state_.engineeringCapacity = scenarioManager.definition().engineeringCapacity;
    const EngineeringCapacity previewBonus = activeCapacityBonus(state_);
    state_.previewEngineeringCapacity = displayCapacityWithBonus(state_.engineeringCapacity, previewBonus);
    state_.engineeringCapacityPreviewVisible = hasActiveCapacityPreview(state_);
    updateActionObservations(simulation);
    updateMetricHistory(simulation);
    const UiFrameView view = buildUiFrameView(simulation, scenarioManager);
    hudPanel_.update(context, view, view.scenario, packManager);
    metricsPanel_.update(context, view);
    selectionPanel_.update(context, view);
    actionPanel_.update(context, view);
    timelinePanel_.update(context, view, view.scenario);
    debugPanel_.update(context, view);
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

void UiManager::draw(const Simulation& simulation, const ScenarioManager& scenarioManager, const content::ContentPackManager& packManager, bool paused) const
{
    UiState* mutableState = const_cast<UiState*>(&state_);
    UiContext context{mutableState, GetScreenWidth(), GetScreenHeight(), paused};
    const UiFrameView view = buildUiFrameView(simulation, scenarioManager);
    metricsPanel_.draw(context, view);
    actionPanel_.draw(context, view);
    timelinePanel_.draw(context, view, view.scenario);
    selectionPanel_.draw(context, view);
    debugPanel_.draw(context, view);
    actionPanel_.drawPlanningOverlays(context, view.scenario);
    hudPanel_.draw(context, view, view.scenario, packManager);
    drawDockResizeHandles(computeDockLayout(state_.dockLayout, GetScreenWidth(), GetScreenHeight()), state_.dockLayout);
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
