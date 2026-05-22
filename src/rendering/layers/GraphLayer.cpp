#include "rendering/Renderer.hpp"

#include "rendering/layers/RendererLayerUtils.hpp"
#include "ui/widgets/IconRegistry.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace rendering::layers;

namespace {

void drawTrafficFlow(Vec2 start, Vec2 end, int screenWidth, int screenHeight, const CameraController& camera, const rendering::LinkPresentationState& visual)
{
    if (visual.activity < 0.03f) {
        return;
    }

    const float load = std::clamp(visual.load, 0.0f, 1.0f);
    const float congestion = std::clamp(visual.congestion, 0.0f, 1.0f);
    const float activity = std::clamp(visual.activity, 0.0f, 1.0f);

    const float flowThickness = 2.5f + load * 6.0f;
    const Color laneColor{89, 196, 255, static_cast<unsigned char>(50 + activity * 95.0f)};
    drawArc(start, end, screenWidth, screenHeight, camera, flowThickness, laneColor);

    if (congestion > 0.25f) {
        const Color congestionColor{245, 184, 76, static_cast<unsigned char>(35 + congestion * 100.0f)};
        drawArc(start, end, screenWidth, screenHeight, camera, flowThickness + congestion * 5.0f, congestionColor);
    }

    const int particleCount = std::clamp(2 + static_cast<int>(std::round(activity * 7.0f)), 2, 9);
    for (int i = 0; i < particleCount; ++i) {
        const float base = (static_cast<float>(i) / static_cast<float>(particleCount));
        float t = visual.flowOffset + base;
        t = t - std::floor(t);

        const Vec2 world = arcPoint(start, end, t);
        const Vector2 position = worldToScreen(world, screenWidth, screenHeight, camera);
        const float radius = 2.5f + activity * 2.4f + congestion * 1.4f;
        const Color particleColor = congestion > 0.62f
            ? Color{245, 184, 76, static_cast<unsigned char>(135 + activity * 95.0f)}
            : Color{89, 196, 255, static_cast<unsigned char>(135 + activity * 95.0f)};
        DrawCircleV(position, radius, particleColor);
        DrawCircleV(position, radius + 4.0f, {particleColor.r, particleColor.g, particleColor.b, 36});
    }
}

void drawReliabilityDependencyPath(Vec2 start,
                                   Vec2 end,
                                   int screenWidth,
                                   int screenHeight,
                                   const CameraController& camera,
                                   float risk,
                                   bool selected)
{
    const float clampedRisk = std::clamp(risk, 0.0f, 1.0f);
    if (clampedRisk < 0.10f && !selected) {
        return;
    }

    const unsigned char baseAlpha = static_cast<unsigned char>((selected ? 105.0f : 38.0f) + clampedRisk * (selected ? 120.0f : 105.0f));
    const Color dependencyColor = clampedRisk > 0.62f
        ? Color{235, 86, 100, baseAlpha}
        : Color{151, 111, 255, baseAlpha};
    drawArc(start, end, screenWidth, screenHeight, camera, 2.5f + clampedRisk * 4.5f + (selected ? 2.0f : 0.0f), dependencyColor);

    if (clampedRisk > 0.55f) {
        const Vec2 mid = arcPoint(start, end, 0.5f);
        const Vector2 marker = worldToScreen(mid, screenWidth, screenHeight, camera);
        const float markerSize = 5.0f + clampedRisk * 5.0f;
        DrawLineEx({marker.x - markerSize, marker.y - markerSize}, {marker.x + markerSize, marker.y + markerSize}, 2.0f, dependencyColor);
        DrawLineEx({marker.x + markerSize, marker.y - markerSize}, {marker.x - markerSize, marker.y + markerSize}, 2.0f, dependencyColor);
    }
}

}

void Renderer::drawLinks(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    const Simulation& simulation = frame.simulation;
    const GeoLayoutFrame& layout = frame.geoLayout;

    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& link : simulation.graph().links()) {
        if (!link.enabled) {
            continue;
        }
        const Node* source = simulation.graph().node(link.sourceNodeId);
        const Node* target = simulation.graph().node(link.targetNodeId);
        if (source == nullptr || target == nullptr) {
            continue;
        }

        const GeoNodeLayout* sourceLayout = layout.node(source->id);
        const GeoNodeLayout* targetLayout = layout.node(target->id);
        if (sourceLayout == nullptr || targetLayout == nullptr || sourceLayout->hiddenByCluster || targetLayout->hiddenByCluster) {
            continue;
        }
        const float feedback = visualFeedback_.linkThroughputBoost(link.id);
        const float activation = visualFeedback_.linkActivation(link.id);
        const bool trafficView = frame.uiState.activeViewMode == UiViewMode::Traffic;
        const bool reliabilityView = frame.uiState.activeViewMode == UiViewMode::Reliability;
        const auto& linkVisual = frame.presentation.link(link.id);
        const float trafficLoad = trafficView ? linkVisual.load : 0.0f;
        const float sourceRisk = frame.presentation.node(source->id).reliabilityRisk;
        const float targetRisk = frame.presentation.node(target->id).reliabilityRisk;
        const float dependencyRisk = reliabilityView ? std::max(sourceRisk, targetRisk) : 0.0f;
        const float thickness = 2.0f + std::min(5.0f, static_cast<float>(link.inFlightRequests.size()) * 0.08f) + feedback * 2.0f + activation * 2.5f + trafficLoad * 2.5f;
        const Color color{
            static_cast<unsigned char>(std::min(140, 75 + static_cast<int>(feedback * 70.0f + activation * 60.0f + trafficLoad * 45.0f))),
            static_cast<unsigned char>(std::min(196, 94 + static_cast<int>(feedback * 90.0f + activation * 70.0f + trafficLoad * 55.0f))),
            static_cast<unsigned char>(std::min(255, 115 + static_cast<int>(feedback * 90.0f + activation * 80.0f + trafficLoad * 50.0f))),
            static_cast<unsigned char>(std::min(235, 145 + static_cast<int>(feedback * 55.0f + activation * 65.0f + trafficLoad * 80.0f - dependencyRisk * 45.0f))),
        };
        drawArc(sourceLayout->displayPosition, targetLayout->displayPosition, width, height, camera, thickness, color);
        if (trafficView) {
            drawTrafficFlow(sourceLayout->displayPosition, targetLayout->displayPosition, width, height, camera, linkVisual);
        }
        if (reliabilityView) {
            const bool selected = frame.uiState.selection.nodeId == source->id || frame.uiState.selection.nodeId == target->id;
            drawReliabilityDependencyPath(sourceLayout->displayPosition, targetLayout->displayPosition, width, height, camera, dependencyRisk, selected);
        }
    }
}

void Renderer::drawDependencyHighlights(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    const Simulation& simulation = frame.simulation;
    const GeoLayoutFrame& layout = frame.geoLayout;

    const int selectedNodeId = frame.uiState.selection.nodeId;
    if (selectedNodeId < 0 || frame.uiState.activeViewMode == UiViewMode::Reliability) {
        return;
    }

    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    for (const auto& link : simulation.graph().links()) {
        if (!link.enabled || (link.sourceNodeId != selectedNodeId && link.targetNodeId != selectedNodeId)) {
            continue;
        }
        const GeoNodeLayout* sourceLayout = layout.node(link.sourceNodeId);
        const GeoNodeLayout* targetLayout = layout.node(link.targetNodeId);
        if (sourceLayout == nullptr || targetLayout == nullptr || sourceLayout->hiddenByCluster || targetLayout->hiddenByCluster) {
            continue;
        }
        const NodePressure* sourcePressure = simulation.pressureAnalysis().pressureForNode(link.sourceNodeId);
        const NodePressure* targetPressure = simulation.pressureAnalysis().pressureForNode(link.targetNodeId);
        const double pathPressure = std::max(sourcePressure != nullptr ? sourcePressure->instability : 0.0, targetPressure != nullptr ? targetPressure->instability : 0.0);
        const bool outgoing = link.sourceNodeId == selectedNodeId;
        const Color color = pathPressure > 0.55
            ? Color{245, 184, 76, 225}
            : (outgoing ? Color{89, 196, 255, 190} : Color{151, 111, 255, 175});
        drawArc(sourceLayout->displayPosition, targetLayout->displayPosition, width, height, camera, 5.0f + static_cast<float>(pathPressure) * 3.0f, color);
    }
}
