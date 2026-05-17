#pragma once

#include "gameplay/Scenario.hpp"
#include "gameplay/ScenarioManager.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/GeoLayoutSystem.hpp"
#include "rendering/MapRenderer.hpp"
#include "rendering/VisualFeedbackSystem.hpp"
#include "simulation/Simulation.hpp"
#include "ui/UiManager.hpp"

class Renderer {
public:
    explicit Renderer(const ScenarioDefinition& scenario);

    void draw(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused, const CameraController& camera);
    void releaseResources();

private:
    void drawLinks(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout);
    void drawNodes(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout);
    void drawRequests(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout);
    void drawQueueBars(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout);
    void drawClusters(const CameraController& camera, const GeoLayoutFrame& layout);
    void drawLabels(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout);
    void drawMutationPreview(const Simulation& simulation, const CameraController& camera) const;

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
