#pragma once

#include "rendering/CameraController.hpp"
#include "simulation/topology/Node.hpp"

#include "raylib.h"

Vector2 toRaylib(Vec2 value);
Vector2 worldToScreen(Vec2 world, int screenWidth, int screenHeight);
Vector2 worldToScreen(Vec2 world, int screenWidth, int screenHeight, const CameraController& camera);
Vec2 screenToWorld(Vector2 screen, int screenWidth, int screenHeight, const CameraController& camera);
Vec2 lerp(Vec2 a, Vec2 b, float t);
Color colorForHealth(HealthState health);
