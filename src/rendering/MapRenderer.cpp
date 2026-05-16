#include "rendering/MapRenderer.hpp"

#include "rendering/RenderPrimitives.hpp"
#include "simulation/Geography.hpp"

#include "raylib.h"

namespace {
Color kMapFill{16, 23, 31, 255};
Color kMapBorder{70, 86, 104, 150};
Color kGridLine{70, 86, 104, 60};
Color kEquator{89, 196, 255, 55};

Vec2 mapPoint(double latitude, double longitude)
{
    return MapProjection::projectEquirectangular({latitude, longitude, ""});
}
}

MapRenderer::MapRenderer()
{
}

MapRenderer::~MapRenderer()
{
    release();
}

void MapRenderer::draw(const CameraController& camera) const
{
    if (!attemptedLoad_) {
        attemptedLoad_ = true;
        if (FileExists("assets/world_map.png")) {
            texture_ = LoadTexture("assets/world_map.png");
            hasTexture_ = texture_.id > 0;
        }
    }

    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    const Vector2 topLeft = worldToScreen({-MapProjection::worldWidth * 0.5f, -MapProjection::worldHeight * 0.5f}, screenWidth, screenHeight, camera);
    const Vector2 bottomRight = worldToScreen({MapProjection::worldWidth * 0.5f, MapProjection::worldHeight * 0.5f}, screenWidth, screenHeight, camera);
    const Rectangle mapRect{
        topLeft.x,
        topLeft.y,
        bottomRight.x - topLeft.x,
        bottomRight.y - topLeft.y,
    };

    if (hasTexture_) {
        DrawTexturePro(
            texture_,
            {0.0f, 0.0f, static_cast<float>(texture_.width), static_cast<float>(texture_.height)},
            mapRect,
            {0.0f, 0.0f},
            0.0f,
            {210, 225, 240, 120});
    } else {
        DrawRectangleRec(mapRect, kMapFill);
    }

    DrawRectangleLinesEx(mapRect, 1.0f, kMapBorder);

    for (int longitude = -150; longitude <= 150; longitude += 30) {
        const Vector2 a = worldToScreen(mapPoint(-75.0, longitude), screenWidth, screenHeight, camera);
        const Vector2 b = worldToScreen(mapPoint(75.0, longitude), screenWidth, screenHeight, camera);
        DrawLineEx(a, b, 1.0f, kGridLine);
    }

    for (int latitude = -60; latitude <= 60; latitude += 30) {
        const Vector2 a = worldToScreen(mapPoint(latitude, -180.0), screenWidth, screenHeight, camera);
        const Vector2 b = worldToScreen(mapPoint(latitude, 180.0), screenWidth, screenHeight, camera);
        DrawLineEx(a, b, 1.0f, latitude == 0 ? kEquator : kGridLine);
    }
}

void MapRenderer::release()
{
    if (hasTexture_) {
        UnloadTexture(texture_);
        texture_ = {};
        hasTexture_ = false;
    }
}
