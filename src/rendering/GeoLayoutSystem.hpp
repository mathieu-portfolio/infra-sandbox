#pragma once

#include "rendering/CameraController.hpp"
#include "simulation/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

#include "raylib.h"

#include <string>
#include <vector>

struct GeoLabelLayout {
    bool visible = false;
    Vec2 position{};
};

struct GeoNodeLayout {
    int nodeId = -1;
    Vec2 anchorPosition{};
    Vec2 displayPosition{};
    GeoLabelLayout label{};
    bool hasOffset = false;
    bool hiddenByCluster = false;
};

struct GeoClusterLayout {
    std::string label;
    std::vector<int> nodeIds;
    Vec2 anchorPosition{};
    Vec2 displayPosition{};
};

struct GeoLayoutFrame {
    std::vector<GeoNodeLayout> nodes;
    std::vector<GeoClusterLayout> clusters;

    [[nodiscard]] const GeoNodeLayout* node(int nodeId) const;
};

class GeoLayoutSystem {
public:
    [[nodiscard]] GeoLayoutFrame compute(const Simulation& simulation, const CameraController& camera, const UiState& state, int screenWidth, int screenHeight) const;

private:
    [[nodiscard]] bool shouldCluster(const CameraController& camera) const;
    [[nodiscard]] bool shouldConsiderLabel(const Node& node, const UiState& state, const CameraController& camera) const;
    void resolveNodeSpacing(GeoLayoutFrame& frame, const CameraController& camera, int screenWidth, int screenHeight) const;
    [[nodiscard]] GeoLabelLayout placeLabel(const Node& node, Vec2 displayPosition, const UiState& state, const CameraController& camera, int screenWidth, int screenHeight, std::vector<Rectangle>& occupiedLabels) const;
};
