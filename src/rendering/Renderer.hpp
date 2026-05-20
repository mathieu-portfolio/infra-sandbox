#pragma once

#include "gameplay/Scenario.hpp"
#include "gameplay/scenario/ScenarioManager.hpp"
#include "content/loading/ContentPackManager.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/GeoLayoutSystem.hpp"
#include "rendering/MapRenderer.hpp"
#include "rendering/VisualFeedbackSystem.hpp"
#include "rendering/viewmodels/RenderFrameView.hpp"
#include "simulation/core/Simulation.hpp"
#include "ui/UiManager.hpp"

class Renderer {
public:
    explicit Renderer(const ScenarioDefinition& scenario);

    void draw(const Simulation& simulation, const ScenarioManager& scenarioManager, const content::ContentPackManager& packManager, bool paused, const CameraController& camera);
    void releaseResources();

private:
    void drawLinks(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera);
    void drawDependencyHighlights(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera);
    void drawNodes(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera);
    void drawRequests(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera);
    void drawQueueBars(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera);
    void drawClusters(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera);
    void drawLabels(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera);
    void drawMutationPreview(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera) const;

public:
    [[nodiscard]] UiManager& uiManager();
    [[nodiscard]] const UiManager& uiManager() const;

private:
    ScenarioDefinition scenarioDefinition_;
    MapRenderer mapRenderer_;
    GeoLayoutSystem geoLayoutSystem_;
    UiManager uiManager_;
    VisualFeedbackSystem visualFeedback_;
};
