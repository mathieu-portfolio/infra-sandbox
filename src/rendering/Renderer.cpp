#include "rendering/Renderer.hpp"

#include "rendering/RenderPrimitives.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace {
Color kBackground{13, 17, 23, 255};
Color kPanel{22, 27, 34, 235};
Color kMutedText{139, 148, 158, 255};
Color kText{230, 237, 243, 255};
Color kLink{75, 94, 115, 170};
Color kParticle{89, 196, 255, 255};
Color kTimeout{235, 86, 100, 190};

float pulse(double timeSeconds, double speed, double phase)
{
    return 0.5f + 0.5f * std::sin(static_cast<float>(timeSeconds * speed + phase));
}

void drawTextLine(const std::string& text, int x, int y, int size, Color color)
{
    DrawText(text.c_str(), x, y, size, color);
}
}

Renderer::Renderer(const ScenarioDefinition& scenario)
    : scenarioDefinition_(scenario)
{
}

void Renderer::draw(const Simulation& simulation, bool paused)
{
    BeginDrawing();
    ClearBackground(kBackground);

    drawLinks(simulation);
    drawRequests(simulation);
    drawNodes(simulation);
    drawQueueBars(simulation);
    drawMetricsOverlay(simulation, paused);
    drawControlsOverlay(simulation);

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
        const Vector2 center = worldToScreen(node.position, width, height);
        const Color healthColor = colorForHealth(node.health);

        if (node.type == NodeType::ClientCluster) {
            const float trafficPulse = pulse(simulation.timeSeconds(), 4.0, node.id);
            const float radius = 28.0f + static_cast<float>(node.requestRatePerSecond) * 2.0f + trafficPulse * 8.0f;
            DrawCircleV(center, radius + 12.0f, {58, 139, 253, 35});
            DrawCircleV(center, radius, {88, 166, 255, 115});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, {152, 195, 255, 220});
        } else if (node.type == NodeType::Service) {
            const float overloadPulse = node.health == HealthState::Healthy ? 0.0f : pulse(simulation.timeSeconds(), 10.0, node.id) * 8.0f;
            const Rectangle rect{center.x - 48.0f - overloadPulse * 0.5f, center.y - 48.0f - overloadPulse * 0.5f, 96.0f + overloadPulse, 96.0f + overloadPulse};
            DrawRectangleRounded(rect, 0.12f, 8, {33, 38, 45, 255});
            DrawRectangleRoundedLines(rect, 0.12f, 8, healthColor);
            DrawCircleV(center, 12.0f + static_cast<float>(node.currentUtilization) * 10.0f, healthColor);
        } else {
            const Rectangle body{center.x - 42.0f, center.y - 36.0f, 84.0f, 72.0f};
            DrawRectangleRounded(body, 0.08f, 8, {36, 41, 47, 255});
            DrawRectangleRoundedLines(body, 0.08f, 8, healthColor);
            DrawLineEx({center.x - 42.0f, center.y - 16.0f}, {center.x + 42.0f, center.y - 16.0f}, 2.0f, {90, 107, 126, 180});
            DrawLineEx({center.x - 42.0f, center.y + 10.0f}, {center.x + 42.0f, center.y + 10.0f}, 2.0f, {90, 107, 126, 180});
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
            DrawCircleV(position, 4.0f, kParticle);
            DrawCircleV(position, 8.0f, {89, 196, 255, 45});
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

void Renderer::drawMetricsOverlay(const Simulation& simulation, bool paused)
{
    const auto& metrics = simulation.metrics();
    const int x = 18;
    const int y = 18;
    DrawRectangleRounded({10.0f, 10.0f, 330.0f, 214.0f}, 0.04f, 8, kPanel);

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "FPS %d  |  %s", GetFPS(), paused ? "PAUSED" : "RUNNING");
    drawTextLine(buffer, x, y, 18, kText);
    std::snprintf(buffer, sizeof(buffer), "Input rate: %.1f req/s", metrics.inputRatePerSecond);
    drawTextLine(buffer, x, y + 30, 18, kText);
    std::snprintf(buffer, sizeof(buffer), "Queue depth: %d", metrics.backendQueueDepth);
    drawTextLine(buffer, x, y + 56, 18, kText);
    std::snprintf(buffer, sizeof(buffer), "Processed: %.1f req/s", metrics.processedPerSecond);
    drawTextLine(buffer, x, y + 82, 18, kText);
    std::snprintf(buffer, sizeof(buffer), "Avg latency: %.2fs", metrics.averageLatencySeconds);
    drawTextLine(buffer, x, y + 108, 18, kText);
    std::snprintf(buffer, sizeof(buffer), "Timeouts: %.1f req/s", metrics.timeoutRatePerSecond);
    drawTextLine(buffer, x, y + 134, 18, kText);
    std::snprintf(buffer, sizeof(buffer), "Backend utilization: %.0f%%", metrics.backendUtilization * 100.0);
    drawTextLine(buffer, x, y + 160, 18, kText);
    std::snprintf(buffer, sizeof(buffer), "Cache placeholder: %.0f%%", simulation.cacheEfficiency() * 100.0);
    drawTextLine(buffer, x, y + 186, 18, kText);
}

void Renderer::drawControlsOverlay(const Simulation&)
{
    const char* controls = "Space pause | R reset | Up/Down or +/- demand | 1 scale capacity | 2 cache placeholder | 3 reset capacity";
    const int size = 16;
    const int width = MeasureText(controls, size);
    DrawRectangleRounded({12.0f, static_cast<float>(GetScreenHeight() - 42), static_cast<float>(width + 18), 30.0f}, 0.15f, 8, {22, 27, 34, 210});
    DrawText(controls, 21, GetScreenHeight() - 35, size, kMutedText);
}
