#include "rendering/Renderer.hpp"

#include "rendering/RenderPrimitives.hpp"

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
}

Renderer::Renderer(const ScenarioDefinition& scenario)
    : scenarioDefinition_(scenario)
{
}

void Renderer::draw(const Simulation& simulation, bool paused)
{
    uiManager_.update(simulation, paused);

    BeginDrawing();
    ClearBackground(kBackground);

    drawLinks(simulation);
    drawRequests(simulation);
    drawNodes(simulation);
    drawQueueBars(simulation);
    uiManager_.draw(simulation, paused);

    EndDrawing();
}

void Renderer::drawLinks(const Simulation& simulation)
{
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& link : simulation.graph().links()) {
        const Node* source = simulation.graph().node(link.sourceNodeId);
        const Node* target = simulation.graph().node(link.targetNodeId);
        if (source == nullptr || target == nullptr) {
            continue;
        }

        const Vector2 start = worldToScreen(source->position, width, height);
        const Vector2 end = worldToScreen(target->position, width, height);
        const float thickness = 2.0f + std::min(5.0f, static_cast<float>(link.inFlightRequests.size()) * 0.08f);
        DrawLineEx(start, end, thickness, kLink);
    }
}

void Renderer::drawNodes(const Simulation& simulation)
{
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& node : simulation.graph().nodes()) {
        const auto& definition = NodeRegistry::definition(node.type);
        const Vector2 center = worldToScreen(node.position, width, height);
        const Color healthColor = colorForHealth(node.health);
        const Color definitionColor = toRaylib(definition.color);
        const Color overlayTint = uiManager_.overlayController().nodeTint(node, simulation, uiManager_.state());

        if (definition.renderStyle == NodeRenderStyle::DemandPulse) {
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

        DrawText(node.name.c_str(), static_cast<int>(center.x - MeasureText(node.name.c_str(), 18) * 0.5f), static_cast<int>(center.y + 62.0f), 18, kText);
    }
}

void Renderer::drawRequests(const Simulation& simulation)
{
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& [id, request] : simulation.requests()) {
        (void)id;

        if (request.state == RequestState::InTransit && request.currentLinkId >= 0) {
            const Link* link = simulation.graph().link(request.currentLinkId);
            if (link == nullptr) {
                continue;
            }

            const Node* source = simulation.graph().node(link->sourceNodeId);
            const Node* target = simulation.graph().node(link->targetNodeId);
            if (source == nullptr || target == nullptr) {
                continue;
            }

            const Vec2 world = lerp(source->position, target->position, static_cast<float>(request.transitProgress));
            const Vector2 position = worldToScreen(world, width, height);
            const bool databaseHeavy = request.type == RequestType::DatabaseHeavy;
            const Color color = request.servedFromCache ? kCacheParticle : (databaseHeavy ? kDbParticle : kParticle);
            const float radius = databaseHeavy ? 5.5f : 4.0f;
            DrawCircleV(position, radius, color);
            DrawCircleV(position, radius + 4.0f, {color.r, color.g, color.b, 45});
        } else if (request.state == RequestState::RetryWaiting) {
            const Node* source = simulation.graph().node(request.sourceNodeId);
            if (source != nullptr) {
                const Vector2 position = worldToScreen(source->position, width, height);
                DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 42.0f, {235, 86, 100, 140});
            }
        } else if (request.state == RequestState::TimedOut && simulation.timeSeconds() - request.completedTime < 0.6) {
            const Node* node = simulation.graph().node(request.currentNodeId);
            if (node != nullptr) {
                const Vector2 position = worldToScreen(node->position, width, height);
                DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 62.0f, kTimeout);
            }
        }
    }
}

void Renderer::drawQueueBars(const Simulation& simulation)
{
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    for (const auto& node : simulation.graph().nodes()) {
        if (!node.isProcessor()) {
            continue;
        }

        const Vector2 center = worldToScreen(node.position, width, height);
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
