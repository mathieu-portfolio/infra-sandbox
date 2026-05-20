#include "rendering/Renderer.hpp"

#include "rendering/layers/RendererLayerUtils.hpp"
#include "ui/IconRegistry.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace rendering::layers;

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
        const float thickness = 2.0f + std::min(5.0f, static_cast<float>(link.inFlightRequests.size()) * 0.08f) + feedback * 2.0f + activation * 2.5f;
        const Color color{
            static_cast<unsigned char>(std::min(140, 75 + static_cast<int>(feedback * 70.0f + activation * 60.0f))),
            static_cast<unsigned char>(std::min(196, 94 + static_cast<int>(feedback * 90.0f + activation * 70.0f))),
            static_cast<unsigned char>(std::min(255, 115 + static_cast<int>(feedback * 90.0f + activation * 80.0f))),
            static_cast<unsigned char>(std::min(235, 170 + static_cast<int>(feedback * 55.0f + activation * 65.0f))),
        };
        drawArc(sourceLayout->displayPosition, targetLayout->displayPosition, width, height, camera, thickness, color);
    }
}

void Renderer::drawDependencyHighlights(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    const Simulation& simulation = frame.simulation;
    const GeoLayoutFrame& layout = frame.geoLayout;

    const int selectedNodeId = uiManager_.state().selection.nodeId;
    if (selectedNodeId < 0) {
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
