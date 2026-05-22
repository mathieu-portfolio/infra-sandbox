#include "rendering/Renderer.hpp"

#include "rendering/layers/RendererLayerUtils.hpp"
#include "ui/widgets/IconRegistry.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace rendering::layers;


namespace {

float clamp01(double value)
{
    return static_cast<float>(std::clamp(value, 0.0, 1.0));
}

float maxResourcePressure(const rendering::NodePresentationState& visual)
{
    return std::max(std::max(visual.computePressure, visual.memoryPressure), std::max(visual.storagePressure, visual.networkPressure));
}

void drawResourceMeter(Vector2 origin, const char* label, float value, Color color)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    const Rectangle track{origin.x, origin.y, 66.0f, 7.0f};
    DrawRectangleRounded(track, 0.45f, 6, {18, 22, 28, 210});
    if (clamped > 0.01f) {
        DrawRectangleRounded({track.x, track.y, track.width * clamped, track.height}, 0.45f, 6, color);
    }
    DrawText(label, static_cast<int>(origin.x - 18.0f), static_cast<int>(origin.y - 3.0f), 11, {198, 208, 220, 185});
}

void drawResourceOverlay(const rendering::NodePresentationState& visual, Vector2 center)
{
    const float saturation = maxResourcePressure(visual);
    if (saturation < 0.03f) {
        return;
    }

    const float panelWidth = 104.0f;
    const float panelHeight = 48.0f;
    const float left = center.x - panelWidth * 0.5f;
    const float top = center.y + 44.0f;

    const unsigned char panelAlpha = static_cast<unsigned char>(160 + saturation * 65.0f);
    DrawRectangleRounded({left, top, panelWidth, panelHeight}, 0.14f, 8, {12, 16, 21, panelAlpha});
    DrawRectangleRoundedLines({left, top, panelWidth, panelHeight}, 0.14f, 8, {139, 148, 158, static_cast<unsigned char>(65 + saturation * 70.0f)});

    drawResourceMeter({left + 28.0f, top + 7.0f}, "CPU", visual.computePressure, {89, 196, 255, 215});
    drawResourceMeter({left + 28.0f, top + 17.0f}, "MEM", visual.memoryPressure, {151, 111, 255, 215});
    drawResourceMeter({left + 28.0f, top + 27.0f}, "DSK", visual.storagePressure, {86, 210, 151, 215});
    drawResourceMeter({left + 28.0f, top + 37.0f}, "NET", visual.networkPressure, {245, 184, 76, 215});

    if (saturation > 0.68f) {
        const unsigned char heatAlpha = static_cast<unsigned char>((saturation - 0.68f) / 0.32f * 95.0f);
        DrawCircleV(center, 60.0f + saturation * 9.0f, {245, 184, 76, heatAlpha});
    }
}

void drawPersistenceOverlay(const NodeDefinition& definition, const Node& node, const rendering::NodePresentationState& visual, Vector2 center, double timeSeconds)
{
    if (!definition.storesState) {
        return;
    }

    const float sync = 0.45f + 0.55f * pulse(timeSeconds, 3.0, node.id);
    const Color dataColor{86, 210, 151, static_cast<unsigned char>(115 + sync * 90.0f)};
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 50.0f + sync * 5.0f, dataColor);
    DrawRectangleRounded({center.x - 24.0f, center.y - 58.0f, 48.0f, 8.0f}, 0.5f, 6, {18, 22, 28, 210});
    DrawRectangleRounded({center.x - 24.0f, center.y - 58.0f, 48.0f * visual.persistenceHealth, 8.0f}, 0.5f, 6, dataColor);
}

void drawReliabilityOverlay(const Node& node, const rendering::NodePresentationState& visual, Vector2 center, double timeSeconds)
{
    const float risk = visual.reliabilityRisk;
    if (risk < 0.08f) {
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 54.0f, {86, 210, 151, 95});
        return;
    }

    const float flicker = 0.65f + 0.35f * pulse(timeSeconds, 12.0, node.id);
    const Color riskColor{235, 86, 100, static_cast<unsigned char>(90 + risk * flicker * 130.0f)};
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 54.0f + risk * 10.0f, riskColor);
    DrawLineEx({center.x - 18.0f, center.y - 46.0f}, {center.x - 5.0f, center.y - 30.0f}, 2.0f, riskColor);
    DrawLineEx({center.x + 8.0f, center.y + 31.0f}, {center.x + 22.0f, center.y + 47.0f}, 2.0f, riskColor);
}

void drawGeographyOverlay(const rendering::NodePresentationState& visual, Vector2 center)
{
    if (visual.geoPresence < 0.02f) {
        return;
    }

    const auto alpha = static_cast<unsigned char>(std::clamp(visual.geoPresence, 0.0f, 1.0f) * 125.0f);
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 48.0f, {89, 196, 255, static_cast<unsigned char>(alpha * 0.64f)});
    DrawLineEx({center.x, center.y + 36.0f}, {center.x, center.y + 55.0f}, 2.0f, {89, 196, 255, alpha});
    DrawCircleV({center.x, center.y + 59.0f}, 3.0f, {89, 196, 255, static_cast<unsigned char>(std::min(150, static_cast<int>(alpha) + 25))});
}

void drawTrafficOverlay(const Node& node, const rendering::NodePresentationState& visual, Vector2 center, double timeSeconds)
{
    const float traffic = visual.traffic;
    if (traffic < 0.05f) {
        return;
    }

    const float beat = pulse(timeSeconds, 5.0 + traffic * 7.0, node.id);
    const float radius = 48.0f + beat * 8.0f;
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, {89, 196, 255, static_cast<unsigned char>(55 + traffic * 120.0f)});
}

}
void Renderer::drawNodes(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    const Simulation& simulation = frame.simulation;
    const GeoLayoutFrame& layout = frame.geoLayout;

    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& node : simulation.graph().nodes()) {
        const GeoNodeLayout* nodeLayout = layout.node(node.id);
        if (nodeLayout == nullptr || nodeLayout->hiddenByCluster) {
            continue;
        }
        const auto& definition = NodeRegistry::definition(node.type);
        const Vector2 center = worldToScreen(nodeLayout->displayPosition, width, height, camera);
        const Color healthColor = colorForHealth(node.health);
        const Color definitionColor = toRaylib(definition.color);
        const Color overlayTint = uiManager_.overlayController().nodeTint(node, simulation, frame.uiState);
        const std::string iconId = iconIdForNode(node.type);
        const bool hasIcon = IconRegistry::instance().hasIcon(iconId);
        const float actionPulse = visualFeedback_.nodeActionPulse(node.id);
        const float pressureGlow = visualFeedback_.nodePressureGlow(node.id);
        const float instability = visualFeedback_.nodeInstability(node.id);
        const bool drawOverviewStatus = frame.uiState.activeViewMode == UiViewMode::Overview;
        const auto& visual = frame.presentation.node(node.id);
        if (drawOverviewStatus && pressureGlow > 0.02f) {
            const float radius = definition.defaultVisualSize * (0.72f + pressureGlow * 0.55f);
            DrawCircleV(center, radius, {245, 184, 76, static_cast<unsigned char>(std::min(95, static_cast<int>(pressureGlow * 95.0f)))});
        }
        if (drawOverviewStatus && instability > 0.12f) {
            const float flicker = 0.55f + 0.45f * pulse(simulation.timeSeconds(), 18.0, node.id);
            DrawCircleV(center, definition.defaultVisualSize * (0.62f + instability * 0.4f), {235, 86, 100, static_cast<unsigned char>(std::min(80, static_cast<int>(instability * flicker * 80.0f)))});
        }
        if (actionPulse > 0.02f) {
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), definition.defaultVisualSize * (0.58f + (1.0f - actionPulse) * 0.45f), {89, 196, 255, static_cast<unsigned char>(actionPulse * 210.0f)});
        }

        if (hasIcon) {
            const float radius = definition.defaultVisualSize * 0.5f;
            DrawCircleV(center, radius + 12.0f, {definitionColor.r, definitionColor.g, definitionColor.b, 35});
            IconRegistry::instance().drawIcon(iconId, {center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f}, definitionColor);
        } else if (definition.renderStyle == NodeRenderStyle::DemandPulse) {
            const float trafficPulse = pulse(simulation.timeSeconds(), 4.0, node.id);
            const float radius = 28.0f + static_cast<float>(node.requestRatePerSecond) * 2.0f + trafficPulse * 8.0f;
            DrawCircleV(center, radius + 12.0f, {definitionColor.r, definitionColor.g, definitionColor.b, 35});
            DrawCircleV(center, radius, {definitionColor.r, definitionColor.g, definitionColor.b, 115});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, healthColor);
            if (node.stressScore > 0.08) {
                DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius + 8.0f + static_cast<float>(node.stressScore) * 8.0f, {245, 184, 76, static_cast<unsigned char>(90 + node.stressScore * 130.0)});
            }
        } else if (definition.renderStyle == NodeRenderStyle::AccelerationCache) {
            const Rectangle rect{center.x - 42.0f, center.y - 28.0f, 84.0f, 56.0f};
            const Color cacheColor = simulation.cacheEnabled() ? Color{86, 210, 151, 255} : Color{90, 107, 126, 180};
            DrawRectangleRounded(rect, 0.18f, 8, {28, 35, 42, 230});
            DrawRectangleRoundedLines(rect, 0.18f, 8, cacheColor);
            DrawCircleV({center.x - 18.0f, center.y}, 7.0f, cacheColor);
            DrawCircleV({center.x + 4.0f, center.y}, 7.0f, cacheColor);
            DrawCircleV({center.x + 26.0f, center.y}, 7.0f, cacheColor);
        } else if (definition.renderStyle == NodeRenderStyle::ComputeBox) {
            const float overloadPulse = node.health == HealthState::Healthy ? 0.0f : pulse(simulation.timeSeconds(), 10.0, node.id) * 8.0f;
            const Rectangle rect{center.x - 48.0f - overloadPulse * 0.5f, center.y - 48.0f - overloadPulse * 0.5f, 96.0f + overloadPulse, 96.0f + overloadPulse};
            DrawRectangleRounded(rect, 0.12f, 8, {33, 38, 45, 255});
            DrawRectangleRoundedLines(rect, 0.12f, 8, healthColor);
            DrawCircleV(center, 12.0f + static_cast<float>(node.currentUtilization) * 10.0f, healthColor);
        } else if (definition.renderStyle == NodeRenderStyle::PersistenceBlock) {
            const Rectangle body{center.x - 42.0f, center.y - 36.0f, 84.0f, 72.0f};
            DrawRectangleRounded(body, 0.08f, 8, {36, 41, 47, 255});
            DrawRectangleRoundedLines(body, 0.08f, 8, healthColor);
            DrawLineEx({center.x - 42.0f, center.y - 16.0f}, {center.x + 42.0f, center.y - 16.0f}, 2.0f, {90, 107, 126, 180});
            DrawLineEx({center.x - 42.0f, center.y + 10.0f}, {center.x + 42.0f, center.y + 10.0f}, 2.0f, {90, 107, 126, 180});
        } else if (definition.renderStyle == NodeRenderStyle::NetworkingHub) {
            DrawCircleV(center, 34.0f, {32, 38, 45, 240});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 34.0f, definitionColor);
            DrawLineEx({center.x - 22.0f, center.y}, {center.x + 22.0f, center.y}, 2.0f, definitionColor);
            DrawLineEx({center.x, center.y - 22.0f}, {center.x, center.y + 22.0f}, 2.0f, definitionColor);
        } else if (definition.renderStyle == NodeRenderStyle::CoordinationDiamond) {
            drawDiamond(center, 38.0f, {32, 38, 45, 240}, definitionColor);
        } else if (definition.renderStyle == NodeRenderStyle::ObservabilityLens) {
            DrawCircleV(center, 30.0f, {definitionColor.r, definitionColor.g, definitionColor.b, 55});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 30.0f, definitionColor);
        } else {
            const Rectangle rect{center.x - 34.0f, center.y - 34.0f, 68.0f, 68.0f};
            DrawRectangleRounded(rect, 0.08f, 8, {32, 38, 45, 235});
            DrawRectangleRoundedLines(rect, 0.08f, 8, definitionColor);
        }

        if (drawOverviewStatus && overlayTint.a > 0) {
            DrawCircleV(center, 58.0f, overlayTint);
        }

        switch (frame.uiState.activeViewMode) {
        case UiViewMode::Traffic:
            drawTrafficOverlay(node, visual, center, simulation.timeSeconds());
            break;
        case UiViewMode::Resources:
            drawResourceOverlay(visual, center);
            break;
        case UiViewMode::Persistence:
            drawPersistenceOverlay(definition, node, visual, center, simulation.timeSeconds());
            break;
        case UiViewMode::Reliability:
            drawReliabilityOverlay(node, visual, center, simulation.timeSeconds());
            break;
        case UiViewMode::Geography:
            drawGeographyOverlay(visual, center);
            break;
        case UiViewMode::Overview:
        case UiViewMode::Count:
            break;
        }

        if (frame.uiState.selection.nodeId == node.id) {
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 64.0f, {230, 237, 243, 230});
        }

        if (nodeLayout->hasOffset) {
            const Vector2 anchor = worldToScreen(nodeLayout->anchorPosition, width, height, camera);
            DrawLineEx(anchor, center, 1.0f, {139, 148, 158, 80});
            DrawCircleV(anchor, 3.0f, {139, 148, 158, 120});
        }
    }
}

void Renderer::drawQueueBars(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    const Simulation& simulation = frame.simulation;
    const GeoLayoutFrame& layout = frame.geoLayout;

    if (frame.uiState.activeViewMode != UiViewMode::Traffic) {
        return;
    }

    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& node : simulation.graph().nodes()) {
        if (!node.isProcessor()) {
            continue;
        }
        if (node.queue.empty()) {
            continue;
        }
        const GeoNodeLayout* nodeLayout = layout.node(node.id);
        if (nodeLayout == nullptr || nodeLayout->hiddenByCluster) {
            continue;
        }

        const Vector2 center = worldToScreen(nodeLayout->displayPosition, width, height, camera);
        const int visibleDots = std::min(20, static_cast<int>(node.queue.size()));
        const float x = center.x + 70.0f;
        const float bottom = center.y + 48.0f;

        const float pressureGlow = frame.presentation.node(node.id).queuePressure;
        DrawRectangleRounded({x - 8.0f, bottom - 122.0f, 16.0f, 128.0f}, 0.35f, 8, {18, 22, 28, static_cast<unsigned char>(220 + std::min(30, static_cast<int>(pressureGlow * 30.0f)))});
        for (int i = 0; i < visibleDots; ++i) {
            const float queueWave = pressureGlow > 0.15f ? std::sin(static_cast<float>(simulation.timeSeconds() * 8.0 + i)) * pressureGlow * 1.6f : 0.0f;
            const float y = bottom - static_cast<float>(i) * 6.0f + queueWave;
            const Color color = i > 12 ? Color{235, 86, 100, 255} : Color{245, 184, 76, 255};
            DrawCircleV({x, y}, 3.0f, color);
        }

        if (node.queue.size() > 20) {
            DrawText("+", static_cast<int>(x - 5.0f), static_cast<int>(bottom - 142.0f), 18, kTimeout);
        }
    }
}

void Renderer::drawClusters(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    const GeoLayoutFrame& layout = frame.geoLayout;

    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    for (const auto& cluster : layout.clusters) {
        const Vector2 center = worldToScreen(cluster.displayPosition, width, height, camera);
        const float radius = 22.0f + static_cast<float>(cluster.nodeIds.size()) * 3.0f;
        DrawCircleV(center, radius + 10.0f, {89, 196, 255, 28});
        DrawCircleV(center, radius, {32, 38, 45, 235});
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, {89, 196, 255, 190});
        const std::string count = std::to_string(cluster.nodeIds.size());
        DrawText(count.c_str(), static_cast<int>(center.x - MeasureText(count.c_str(), 18) * 0.5f), static_cast<int>(center.y - 9.0f), 18, kText);
        DrawText(cluster.label.c_str(), static_cast<int>(center.x - MeasureText(cluster.label.c_str(), 14) * 0.5f), static_cast<int>(center.y + radius + 8.0f), 14, {139, 148, 158, 180});
    }
}

void Renderer::drawLabels(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera)
{
    const Simulation& simulation = frame.simulation;
    const GeoLayoutFrame& layout = frame.geoLayout;

    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    for (const auto& nodeLayout : layout.nodes) {
        if (!nodeLayout.label.visible || nodeLayout.hiddenByCluster) {
            continue;
        }
        const Node* node = simulation.graph().node(nodeLayout.nodeId);
        if (node == nullptr) {
            continue;
        }
        const Vector2 label = worldToScreen(nodeLayout.label.position, width, height, camera);
        const int textWidth = MeasureText(node->name.c_str(), 16);
        DrawRectangleRounded({label.x - 5.0f, label.y - 3.0f, static_cast<float>(textWidth) + 10.0f, 22.0f}, 0.18f, 6, {13, 17, 23, 180});
        DrawText(node->name.c_str(), static_cast<int>(label.x), static_cast<int>(label.y), 16, kText);
    }
}
