#pragma once

#include "rendering/CameraController.hpp"
#include "rendering/GeoLayoutSystem.hpp"
#include "rendering/RenderPrimitives.hpp"
#include "simulation/topology/Geography.hpp"
#include "simulation/topology/TopologyMutation.hpp"
#include "simulation/core/Simulation.hpp"
#include "ui/actions/ActionPanelModel.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace rendering::layers {

inline constexpr Color kBackground{13, 17, 23, 255};
inline constexpr Color kText{230, 237, 243, 255};
inline constexpr Color kLink{75, 94, 115, 170};
inline constexpr Color kParticle{89, 196, 255, 255};
inline constexpr Color kDbParticle{245, 184, 76, 255};
inline constexpr Color kCacheParticle{86, 210, 151, 255};
inline constexpr Color kTimeout{235, 86, 100, 190};

inline float pulse(double timeSeconds, double speed, double phase)
{
    return 0.5f + 0.5f * std::sin(static_cast<float>(timeSeconds * speed + phase));
}

inline Color toRaylib(NodeVisualColor color)
{
    return {color.r, color.g, color.b, color.a};
}

inline Rectangle mapBounds(const CameraController& camera, int screenWidth, int screenHeight)
{
    const Vector2 topLeft = worldToScreen({-MapProjection::worldWidth * 0.5f, -MapProjection::worldHeight * 0.5f}, screenWidth, screenHeight, camera);
    const Vector2 bottomRight = worldToScreen({MapProjection::worldWidth * 0.5f, MapProjection::worldHeight * 0.5f}, screenWidth, screenHeight, camera);
    return {topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y};
}

inline int hoveredPlacementCandidateIndex(const Simulation& simulation, TopologyMutationType type, const CameraController& camera, Vector2 mousePosition, int screenWidth, int screenHeight)
{
    if (!CheckCollisionPointRec(mousePosition, mapBounds(camera, screenWidth, screenHeight))) {
        return -1;
    }
    const PlacementCandidateGenerator generator;
    const auto candidates = generator.generate(simulation, type);
    int bestIndex = -1;
    float bestDistanceSquared = 1.0e12f;
    for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
        const Vec2 world = MapProjection::projectEquirectangular(candidates[static_cast<std::size_t>(i)].location);
        const Vector2 center = worldToScreen(world, screenWidth, screenHeight, camera);
        const float dx = mousePosition.x - center.x;
        const float dy = mousePosition.y - center.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestIndex = i;
        }
    }
    return bestIndex;
}

inline void drawDiamond(Vector2 center, float radius, Color fill, Color outline)
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

inline Vec2 arcPoint(Vec2 start, Vec2 end, float t)
{
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    const Vec2 linear = lerp(start, end, clamped);
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float lift = std::clamp(distance * 0.18f, 18.0f, 105.0f) * 4.0f * clamped * (1.0f - clamped);
    return {linear.x, linear.y - lift};
}

inline void drawArc(Vec2 start, Vec2 end, int screenWidth, int screenHeight, const CameraController& camera, float thickness, Color color)
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

inline std::string iconIdForNode(NodeType type)
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

} // namespace rendering::layers
