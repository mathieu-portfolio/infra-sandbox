#include "rendering/Renderer.hpp"

#include "rendering/RenderPrimitives.hpp"
#include "simulation/Geography.hpp"
#include "ui/ActionPanelModel.hpp"
#include "ui/IconRegistry.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
Color kBackground{13, 17, 23, 255};
Color kText{230, 237, 243, 255};
Color kLink{75, 94, 115, 170};
Color kParticle{89, 196, 255, 255};
Color kDbParticle{245, 184, 76, 255};
Color kCacheParticle{86, 210, 151, 255};
Color kTimeout{235, 86, 100, 190};

float pulse(double timeSeconds, double speed, double phase)
{
    return 0.5f + 0.5f * std::sin(static_cast<float>(timeSeconds * speed + phase));
}

Color toRaylib(NodeVisualColor color)
{
    return {color.r, color.g, color.b, color.a};
}

void drawDiamond(Vector2 center, float radius, Color fill, Color outline)
{
    const Vector2 points[4] = {
        {center.x, center.y - radius},
        {center.x + radius, center.y},
        {center.x, center.y + radius},
        {center.x - radius, center.y},
    };
    DrawTriangle(points[0], points[1], points[2], fill);
    DrawTriangle(points[0], points[2], points[3], fill);
    DrawLineEx(points[0], points[1], 2.0f, outline);
    DrawLineEx(points[1], points[2], 2.0f, outline);
    DrawLineEx(points[2], points[3], 2.0f, outline);
    DrawLineEx(points[3], points[0], 2.0f, outline);
}

Vec2 arcPoint(Vec2 start, Vec2 end, float t)
{
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    const Vec2 linear = lerp(start, end, clamped);
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float lift = std::clamp(distance * 0.18f, 18.0f, 105.0f) * 4.0f * clamped * (1.0f - clamped);
    return {linear.x, linear.y - lift};
}

void drawArc(Vec2 start, Vec2 end, int screenWidth, int screenHeight, const CameraController& camera, float thickness, Color color)
{
    constexpr int segments = 18;
    Vector2 previous = worldToScreen(arcPoint(start, end, 0.0f), screenWidth, screenHeight, camera);
    for (int i = 1; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const Vector2 current = worldToScreen(arcPoint(start, end, t), screenWidth, screenHeight, camera);
        DrawLineEx(previous, current, thickness, color);
        previous = current;
    }
}

std::string iconIdForNode(NodeType type)
{
    switch (type) {
    case NodeType::ClientCluster:
        return "node.client";
    case NodeType::ApiService:
    case NodeType::Microservice:
        return "node.service";
    case NodeType::Database:
    case NodeType::ReadReplica:
        return "node.database";
    case NodeType::Cache:
    case NodeType::CDNEdge:
        return "node.cache";
    case NodeType::QueueBroker:
        return "node.queue";
    default:
        return "node.generic";
    }
}
}

Renderer::Renderer(const ScenarioDefinition& scenario)
    : scenarioDefinition_(scenario)
{
}

void Renderer::draw(const Simulation& simulation, const ScenarioManager& scenarioManager, bool paused, const CameraController& camera)
{
    uiManager_.update(simulation, scenarioManager, paused);

    BeginDrawing();
    ClearBackground(kBackground);

    const GeoLayoutFrame geoLayout = geoLayoutSystem_.compute(simulation, camera, uiManager_.state(), GetScreenWidth(), GetScreenHeight());
    mapRenderer_.draw(camera, uiManager_.state().showGeoGrid);
    drawMutationPreview(simulation, camera);
    drawLinks(simulation, camera, geoLayout);
    drawRequests(simulation, camera, geoLayout);
    drawClusters(camera, geoLayout);
    drawNodes(simulation, camera, geoLayout);
    drawQueueBars(simulation, camera, geoLayout);
    drawLabels(simulation, camera, geoLayout);
    uiManager_.draw(simulation, scenarioManager, paused);

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

void Renderer::drawLinks(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout)
{
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
        const float thickness = 2.0f + std::min(5.0f, static_cast<float>(link.inFlightRequests.size()) * 0.08f);
        drawArc(sourceLayout->displayPosition, targetLayout->displayPosition, width, height, camera, thickness, kLink);
    }
}

void Renderer::drawNodes(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout)
{
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
        const Color overlayTint = uiManager_.overlayController().nodeTint(node, simulation, uiManager_.state());
        const std::string iconId = iconIdForNode(node.type);
        const bool hasIcon = IconRegistry::instance().hasIcon(iconId);

        if (hasIcon) {
            const float radius = definition.defaultVisualSize * 0.5f;
            DrawCircleV(center, radius + 12.0f, {definitionColor.r, definitionColor.g, definitionColor.b, 35});
            IconRegistry::instance().drawIcon(iconId, {center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f}, definitionColor);
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius + 8.0f, healthColor);
        } else if (definition.renderStyle == NodeRenderStyle::DemandPulse) {
            const float trafficPulse = pulse(simulation.timeSeconds(), 4.0, node.id);
            const float radius = 28.0f + static_cast<float>(node.requestRatePerSecond) * 2.0f + trafficPulse * 8.0f;
            DrawCircleV(center, radius + 12.0f, {definitionColor.r, definitionColor.g, definitionColor.b, 35});
            DrawCircleV(center, radius, {definitionColor.r, definitionColor.g, definitionColor.b, 115});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, {152, 195, 255, 220});
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

        if (overlayTint.a > 0) {
            DrawCircleV(center, 58.0f, overlayTint);
        }

        if (uiManager_.state().selection.nodeId == node.id) {
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 64.0f, {230, 237, 243, 230});
        }

        if (nodeLayout->hasOffset) {
            const Vector2 anchor = worldToScreen(nodeLayout->anchorPosition, width, height, camera);
            DrawLineEx(anchor, center, 1.0f, {139, 148, 158, 80});
            DrawCircleV(anchor, 3.0f, {139, 148, 158, 120});
        }
    }
}

void Renderer::drawRequests(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout)
{
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
            const Color color = request.servedFromCache ? kCacheParticle : (databaseHeavy ? kDbParticle : kParticle);
            const float radius = databaseHeavy ? 5.5f : 4.0f;
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

void Renderer::drawQueueBars(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout)
{
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& node : simulation.graph().nodes()) {
        if (!node.isProcessor()) {
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

        DrawRectangleRounded({x - 8.0f, bottom - 122.0f, 16.0f, 128.0f}, 0.35f, 8, {18, 22, 28, 220});
        for (int i = 0; i < visibleDots; ++i) {
            const float y = bottom - static_cast<float>(i) * 6.0f;
            const Color color = i > 12 ? Color{235, 86, 100, 255} : Color{245, 184, 76, 255};
            DrawCircleV({x, y}, 3.0f, color);
        }

        if (node.queue.size() > 20) {
            DrawText("+", static_cast<int>(x - 5.0f), static_cast<int>(bottom - 142.0f), 18, kTimeout);
        }
    }
}

void Renderer::drawClusters(const CameraController& camera, const GeoLayoutFrame& layout)
{
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

void Renderer::drawLabels(const Simulation& simulation, const CameraController& camera, const GeoLayoutFrame& layout)
{
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

void Renderer::drawMutationPreview(const Simulation& simulation, const CameraController& camera) const
{
    const UiState& state = uiManager_.state();
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    const PlacementCandidateGenerator generator;
    const MutationValidator validator;
    TopologyMutationType mutation = state.activeMutation;
    bool activePreview = state.placementActive;
    if (!activePreview) {
        const ActionPanelModel model;
        const auto cards = model.buildCards(simulation, state, width, height);
        const int index = state.hoveredActionIndex >= 0 ? state.hoveredActionIndex : state.selectedActionIndex;
        if (index >= 0 && index < static_cast<int>(cards.size()) && cards[static_cast<std::size_t>(index)].kind == ActionCardKind::TopologyMutation && cards[static_cast<std::size_t>(index)].available) {
            mutation = cards[static_cast<std::size_t>(index)].mutation;
            activePreview = true;
        }
    }
    if (!activePreview) {
        return;
    }

    const auto candidates = generator.generate(simulation, mutation);
    if (candidates.empty()) {
        return;
    }

    const int selected = state.placementActive ? std::clamp(state.placementCandidateIndex, 0, static_cast<int>(candidates.size()) - 1) : 0;
    for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
        const Vec2 world = MapProjection::projectEquirectangular(candidates[static_cast<std::size_t>(i)].location);
        const Vector2 center = worldToScreen(world, width, height, camera);
        const bool isSelected = i == selected;
        const Color color = isSelected ? Color{89, 196, 255, 210} : Color{139, 148, 158, 120};
        DrawCircleV(center, isSelected ? 38.0f : 28.0f, {color.r, color.g, color.b, 34});
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), isSelected ? 38.0f : 28.0f, color);
        DrawText(candidates[static_cast<std::size_t>(i)].displayName.c_str(), static_cast<int>(center.x + 42.0f), static_cast<int>(center.y - 8.0f), 14, color);
    }

    const auto& option = candidates[static_cast<std::size_t>(selected)];
    const MutationPreview preview = validator.preview(simulation, mutation, option);
    const Color ghost{89, 196, 255, 150};
    for (const auto& node : preview.mutation.nodesToCreate) {
        const Vector2 center = worldToScreen(node.position, width, height, camera);
        DrawCircleV(center, 32.0f, {ghost.r, ghost.g, ghost.b, 42});
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 32.0f, ghost);
        DrawText(node.name.c_str(), static_cast<int>(center.x + 38.0f), static_cast<int>(center.y + 18.0f), 14, ghost);
    }

    for (const auto& link : preview.mutation.linksToCreate) {
        Vec2 source{};
        Vec2 target{};
        bool hasSource = false;
        bool hasTarget = false;
        if (const Node* sourceNode = simulation.graph().node(link.sourceNodeId)) {
            source = sourceNode->position;
            hasSource = true;
        } else if (!preview.mutation.nodesToCreate.empty()) {
            source = preview.mutation.nodesToCreate.front().position;
            hasSource = true;
        }
        if (const Node* targetNode = simulation.graph().node(link.targetNodeId)) {
            target = targetNode->position;
            hasTarget = true;
        } else if (!preview.mutation.nodesToCreate.empty()) {
            target = preview.mutation.nodesToCreate.front().position;
            hasTarget = true;
        }
        if (hasSource && hasTarget) {
            drawArc(source, target, width, height, camera, 2.0f, {89, 196, 255, 110});
        }
    }
}
