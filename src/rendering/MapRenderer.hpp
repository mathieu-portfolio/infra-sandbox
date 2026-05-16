#pragma once

#include "rendering/CameraController.hpp"

#include "raylib.h"

class MapRenderer {
public:
    MapRenderer();
    ~MapRenderer();

    MapRenderer(const MapRenderer&) = delete;
    MapRenderer& operator=(const MapRenderer&) = delete;

    void draw(const CameraController& camera, bool showGeoGrid) const;
    void release();

private:
    mutable Texture2D texture_{};
    mutable bool hasTexture_ = false;
    mutable bool attemptedLoad_ = false;
};
