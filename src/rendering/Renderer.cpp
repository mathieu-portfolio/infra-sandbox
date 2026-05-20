#include "rendering/Renderer.hpp"

#include "rendering/layers/RendererLayerUtils.hpp"
#include "rendering/viewmodels/RenderFrameView.hpp"
#include "simulation/topology/Geography.hpp"
#include "ui/actions/ActionPanelModel.hpp"
#include "ui/widgets/IconRegistry.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>


Renderer::Renderer(const ScenarioDefinition& scenario)
    : scenarioDefinition_(scenario)
{
}

void Renderer::draw(const Simulation& simulation, const ScenarioManager& scenarioManager, const content::ContentPackManager& packManager, bool paused, const CameraController& camera)
{
    uiManager_.update(simulation, scenarioManager, packManager, paused);
    for (const auto& event : uiManager_.state().pendingVisualFeedbackEvents) {
        visualFeedback_.submit(event);
    }
    uiManager_.state().pendingVisualFeedbackEvents.clear();
    visualFeedback_.update(GetFrameTime() * simulation.simulationSpeed(), simulation);

    BeginDrawing();
    ClearBackground(rendering::layers::kBackground);

    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    const GeoLayoutFrame geoLayout = geoLayoutSystem_.compute(simulation, camera, uiManager_.state(), screenWidth, screenHeight);
    const rendering::viewmodels::RenderFrameView frameView{simulation, uiManager_.state(), geoLayout, screenWidth, screenHeight};
    mapRenderer_.draw(camera, uiManager_.state().showGeoGrid);
    drawMutationPreview(frameView, camera);
    drawDependencyHighlights(frameView, camera);
    drawLinks(frameView, camera);
    drawRequests(frameView, camera);
    drawClusters(frameView, camera);
    drawNodes(frameView, camera);
    drawQueueBars(frameView, camera);
    drawLabels(frameView, camera);
    uiManager_.draw(simulation, scenarioManager, packManager, paused);

    EndDrawing();
}

void Renderer::releaseResources()
{
    mapRenderer_.release();
    uiManager_.releaseResources();
}

UiManager& Renderer::uiManager()
{
    return uiManager_;
}

const UiManager& Renderer::uiManager() const
{
    return uiManager_;
}


