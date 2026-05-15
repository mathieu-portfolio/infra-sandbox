#include "rendering/RenderPrimitives.hpp"

#include <algorithm>

Vector2 toRaylib(Vec2 value)
{
    return {value.x, value.y};
}

Vector2 worldToScreen(Vec2 world, int screenWidth, int screenHeight)
{
    return {
        static_cast<float>(screenWidth) * 0.5f + world.x,
        static_cast<float>(screenHeight) * 0.5f + world.y,
    };
}

Vec2 lerp(Vec2 a, Vec2 b, float t)
{
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    return {
        a.x + (b.x - a.x) * clamped,
        a.y + (b.y - a.y) * clamped,
    };
}

Color colorForHealth(HealthState health)
{
    switch (health) {
    case HealthState::Healthy:
        return {86, 210, 151, 255};
    case HealthState::Saturated:
        return {245, 184, 76, 255};
    case HealthState::Failing:
        return {235, 86, 100, 255};
    }
    return RAYWHITE;
}
