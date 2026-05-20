#include "rendering/Renderer.hpp"

#include "rendering/layers/RendererLayerUtils.hpp"
#include "ui/IconRegistry.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace rendering::layers;

void Renderer::drawRequests(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    const Simulation& simulation = frame.simulation;
    const GeoLayoutFrame& layout = frame.geoLayout;

    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& [id, request] : simulation.requests()) {
        (void)id;

        if (request.state == RequestState::InTransit && request.currentLinkId >= 0) {
            const Link* link = simulation.graph().link(request.currentLinkId);
            if (link == nullptr || !link->enabled) {
                continue;
            }

            const Node* source = simulation.graph().node(link->sourceNodeId);
            const Node* target = simulation.graph().node(link->targetNodeId);
            if (source == nullptr || target == nullptr) {
                continue;
            }
            const GeoNodeLayout* sourceLayout = layout.node(source->id);
            const GeoNodeLayout* targetLayout = layout.node(target->id);
            if (sourceLayout == nullptr || targetLayout == nullptr || sourceLayout->hiddenByCluster || targetLayout->hiddenByCluster) {
                continue;
            }

            const Vec2 world = arcPoint(sourceLayout->displayPosition, targetLayout->displayPosition, static_cast<float>(request.transitProgress));
            const Vector2 position = worldToScreen(world, width, height, camera);
        const bool databaseHeavy = request.type == RequestType::DatabaseHeavy;
            Color color = request.servedFromCache ? kCacheParticle : (databaseHeavy ? kDbParticle : kParticle);
            if (visualFeedback_.routeShift() > 0.05f && request.servedFromCache) {
                color.a = static_cast<unsigned char>(std::min(255, 190 + static_cast<int>(visualFeedback_.routeShift() * 65.0f)));
            }
            const float radius = (databaseHeavy ? 5.5f : 4.0f) + (request.servedFromCache ? visualFeedback_.routeShift() * 1.8f : 0.0f);
            DrawCircleV(position, radius, color);
            DrawCircleV(position, radius + 4.0f, {color.r, color.g, color.b, 45});
        } else if (request.state == RequestState::RetryWaiting) {
            const Node* source = simulation.graph().node(request.sourceNodeId);
            if (source != nullptr) {
                const GeoNodeLayout* sourceLayout = layout.node(source->id);
                if (sourceLayout == nullptr || sourceLayout->hiddenByCluster) {
                    continue;
                }
                const Vector2 position = worldToScreen(sourceLayout->displayPosition, width, height, camera);
                DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 42.0f, {235, 86, 100, 140});
            }
        } else if (request.state == RequestState::TimedOut && simulation.timeSeconds() - request.completedTime < 0.6) {
            const Node* node = simulation.graph().node(request.currentNodeId);
            if (node != nullptr) {
                const GeoNodeLayout* nodeLayout = layout.node(node->id);
                if (nodeLayout == nullptr || nodeLayout->hiddenByCluster) {
                    continue;
                }
                const Vector2 position = worldToScreen(nodeLayout->displayPosition, width, height, camera);
                DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 62.0f, kTimeout);
            }
        }
    }
}
