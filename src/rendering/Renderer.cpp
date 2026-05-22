#include "rendering/Renderer.hpp"

#include "rendering/layers/RendererLayerUtils.hpp"
#include "rendering/viewmodels/RenderFrameView.hpp"
#include "core/topology/Geography.hpp"
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

    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    const GeoLayoutFrame geoLayout = geoLayoutSystem_.compute(simulation, camera, uiManager_.state(), screenWidth, screenHeight);
    const UiState& state = uiManager_.state();
    const bool crossfadingViews = state.viewTransition.active();
    bool transitionTargetsReady = false;

    if (crossfadingViews) {
        ensureTransitionTargets(screenWidth, screenHeight);
        transitionTargetsReady = hasTransitionTargets_;
        if (transitionTargetsReady) {
            UiState previousState = state;
            previousState.activeViewMode = state.viewTransition.previous;
            previousState.activeOverlay = overlayForViewMode(state.viewTransition.previous);
            previousState.viewTransition.progress = 1.0f;
            previousState.viewTransition.current = state.viewTransition.previous;
            previousState.viewTransition.previous = state.viewTransition.previous;

            UiState currentState = state;
            currentState.activeViewMode = state.viewTransition.current;
            currentState.activeOverlay = overlayForViewMode(state.viewTransition.current);
            currentState.viewTransition.progress = 1.0f;
            currentState.viewTransition.previous = state.viewTransition.current;
            currentState.viewTransition.current = state.viewTransition.current;

            const rendering::viewmodels::RenderFrameView previousFrame{simulation, previousState, geoLayout, screenWidth, screenHeight};
            const rendering::viewmodels::RenderFrameView currentFrame{simulation, currentState, geoLayout, screenWidth, screenHeight};
            drawWorldToTexture(previousViewTarget_, previousFrame, camera);
            drawWorldToTexture(currentViewTarget_, currentFrame, camera);
        }
    }

    BeginDrawing();
    ClearBackground(rendering::layers::kBackground);

    if (crossfadingViews && transitionTargetsReady) {
        drawTextureFullscreen(previousViewTarget_.texture, 1.0f - state.viewTransition.progress);
        drawTextureFullscreen(currentViewTarget_.texture, state.viewTransition.progress);
    } else {
        const rendering::viewmodels::RenderFrameView frameView{simulation, state, geoLayout, screenWidth, screenHeight};
        drawWorld(frameView, camera);
    }

    uiManager_.draw(simulation, scenarioManager, packManager, paused);

    EndDrawing();
}

void Renderer::drawWorld(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    mapRenderer_.draw(camera, frame.uiState.showGeoGrid);
    drawMutationPreview(frame, camera);
    drawDependencyHighlights(frame, camera);
    drawLinks(frame, camera);
    drawRequests(frame, camera);
    drawClusters(frame, camera);
    drawNodes(frame, camera);
    drawQueueBars(frame, camera);
    drawLabels(frame, camera);
}

void Renderer::drawWorldToTexture(RenderTexture2D& target, const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    BeginTextureMode(target);
    ClearBackground(rendering::layers::kBackground);
    drawWorld(frame, camera);
    EndTextureMode();
}

void Renderer::ensureTransitionTargets(int width, int height)
{
    if (hasTransitionTargets_ && transitionTargetWidth_ == width && transitionTargetHeight_ == height) {
        return;
    }

    releaseTransitionTargets();
    previousViewTarget_ = LoadRenderTexture(width, height);
    currentViewTarget_ = LoadRenderTexture(width, height);
    transitionTargetWidth_ = width;
    transitionTargetHeight_ = height;
    hasTransitionTargets_ = previousViewTarget_.id > 0 && currentViewTarget_.id > 0;
}

void Renderer::releaseTransitionTargets()
{
    if (previousViewTarget_.id > 0) {
        UnloadRenderTexture(previousViewTarget_);
    }
    if (currentViewTarget_.id > 0) {
        UnloadRenderTexture(currentViewTarget_);
    }
    previousViewTarget_ = {};
    currentViewTarget_ = {};
    transitionTargetWidth_ = 0;
    transitionTargetHeight_ = 0;
    hasTransitionTargets_ = false;
}

void Renderer::drawTextureFullscreen(Texture2D texture, float alpha) const
{
    const unsigned char tintAlpha = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
    DrawTexturePro(
        texture,
        {0.0f, 0.0f, static_cast<float>(texture.width), -static_cast<float>(texture.height)},
        {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
        {0.0f, 0.0f},
        0.0f,
        {255, 255, 255, tintAlpha});
}

void Renderer::releaseResources()
{
    releaseTransitionTargets();
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


