#include "ui/UiManager.hpp"

#include "raylib.h"

void UiManager::update(const Simulation& simulation, const ScenarioManager&, bool paused)
{
    UiContext context{&state_, GetScreenWidth(), GetScreenHeight(), paused};
    hudPanel_.update(context, simulation);
    metricsPanel_.update(context, simulation);
    selectionPanel_.update(context, simulation);
    interventionPanel_.update(context, simulation);
    timelinePanel_.update(context, simulation);
    debugPanel_.update(context, simulation);
}

void UiManager::draw(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused) const
{
    UiState* mutableState = const_cast<UiState*>(&state_);
    UiContext context{mutableState, GetScreenWidth(), GetScreenHeight(), paused};
    hudPanel_.draw(context, simulation, scenarioManager);
    metricsPanel_.draw(context, simulation);
    selectionPanel_.draw(context, simulation);
    interventionPanel_.draw(context, simulation);
    timelinePanel_.draw(context, simulation, scenarioManager);
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
