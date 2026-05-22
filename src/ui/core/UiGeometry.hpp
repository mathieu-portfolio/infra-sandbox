#pragma once

struct UiPoint {
    float x{};
    float y{};
};

struct UiRect {
    float x{};
    float y{};
    float width{};
    float height{};
};

[[nodiscard]] inline bool contains(UiRect rect, UiPoint point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.width
        && point.y >= rect.y && point.y <= rect.y + rect.height;
}
