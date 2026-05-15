#pragma once

#include "simulation/Node.hpp"

#include "raylib.h"

Vector2 toRaylib(Vec2 value);
Vector2 worldToScreen(Vec2 world, int screenWidth, int screenHeight);
Vec2 lerp(Vec2 a, Vec2 b, float t);
Color colorForHealth(HealthState health);
