#include "rendering/GeoLayoutSystem.hpp"

#include "rendering/RenderPrimitives.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>

namespace {
bool overlapsAny(Rectangle candidate, const std::vector<Rectangle>& occupied)
{
    return std::any_of(occupied.begin(), occupied.end(), [candidate](Rectangle rect) {
        return CheckCollisionRecs(candidate, rect);
    });
}

Rectangle labelRect(Vector2 origin, const char* text)
{
    return {origin.x - 4.0f, origin.y - 3.0f, static_cast<float>(MeasureText(text, 16)) + 8.0f, 22.0f};
}

Vec2 screenDeltaToWorld(Vector2 delta, const CameraController& camera)
{
    return {delta.x / camera.zoom(), delta.y / camera.zoom()};
}

Vec2 screenToWorld(Vector2 screen, const CameraController& camera, int screenWidth, int screenHeight)
{
    const Vector2 offset = camera.offset();
    return {
        (screen.x - static_cast<float>(screenWidth) * 0.5f) / camera.zoom() - offset.x,
        (screen.y - static_cast<float>(screenHeight) * 0.5f) / camera.zoom() - offset.y,
    };
}

Vector2 add(Vector2 a, Vector2 b)
{
    return {a.x + b.x, a.y + b.y};
}

Vector2 subtract(Vector2 a, Vector2 b)
{
    return {a.x - b.x, a.y - b.y};
}

Vector2 multiply(Vector2 value, float scalar)
{
    return {value.x * scalar, value.y * scalar};
}

float length(Vector2 value)
{
    return std::sqrt(value.x * value.x + value.y * value.y);
}
}

const GeoNodeLayout* GeoLayoutFrame::node(int nodeId) const
{
    const auto it = std::find_if(nodes.begin(), nodes.end(), [nodeId](const GeoNodeLayout& layout) {
        return layout.nodeId == nodeId;
    });
    return it != nodes.end() ? &*it : nullptr;
}

GeoLayoutFrame GeoLayoutSystem::compute(const Simulation& simulation, const CameraController& camera, const UiState& state, int screenWidth, int screenHeight) const
{
    GeoLayoutFrame frame;
    frame.nodes.reserve(simulation.graph().nodes().size());

    for (const auto& node : simulation.graph().nodes()) {
        frame.nodes.push_back({
            .nodeId = node.id,
            .anchorPosition = node.position,
            .displayPosition = node.position,
        });
    }

    if (shouldCluster(camera)) {
        std::unordered_map<std::string, std::vector<int>> byRegion;
        for (const auto& node : simulation.graph().nodes()) {
            if (state.selection.nodeId == node.id || !node.hasGeoLocation) {
                continue;
            }
            byRegion[node.geoLocation.regionName].push_back(node.id);
        }

        for (const auto& [region, nodeIds] : byRegion) {
            if (nodeIds.size() < 2) {
                continue;
            }

            Vec2 average{};
            for (const int nodeId : nodeIds) {
                const Node* node = simulation.graph().node(nodeId);
                if (node == nullptr) {
                    continue;
                }
                average.x += node->position.x;
                average.y += node->position.y;
            }
            average.x /= static_cast<float>(nodeIds.size());
            average.y /= static_cast<float>(nodeIds.size());

            frame.clusters.push_back({region + " x" + std::to_string(nodeIds.size()), nodeIds, average, average});
            for (auto& layout : frame.nodes) {
                if (std::find(nodeIds.begin(), nodeIds.end(), layout.nodeId) != nodeIds.end()) {
                    layout.hiddenByCluster = true;
                }
            }
        }
    }

    resolveNodeSpacing(frame, camera, screenWidth, screenHeight);

    std::vector<Rectangle> occupiedLabels;
    for (auto& layout : frame.nodes) {
        if (layout.hiddenByCluster) {
            continue;
        }
        const Node* node = simulation.graph().node(layout.nodeId);
        if (node == nullptr || !shouldConsiderLabel(*node, state, camera)) {
            continue;
        }
        layout.label = placeLabel(*node, layout.displayPosition, state, camera, screenWidth, screenHeight, occupiedLabels);
    }

    return frame;
}

bool GeoLayoutSystem::shouldCluster(const CameraController& camera) const
{
    return camera.zoom() < 0.62f;
}

bool GeoLayoutSystem::shouldConsiderLabel(const Node& node, const UiState& state, const CameraController& camera) const
{
    if (state.selection.nodeId == node.id) {
        return true;
    }
    if (camera.zoom() < 0.72f) {
        return node.isProcessor();
    }
    if (camera.zoom() < 0.92f) {
        return node.isProcessor() || node.type == NodeType::Cache;
    }
    return true;
}

void GeoLayoutSystem::resolveNodeSpacing(GeoLayoutFrame& frame, const CameraController& camera, int screenWidth, int screenHeight) const
{
    struct WorkingNode {
        std::size_t layoutIndex = 0;
        Vector2 anchor{};
        Vector2 display{};
    };

    std::vector<WorkingNode> working;
    working.reserve(frame.nodes.size());
    for (std::size_t i = 0; i < frame.nodes.size(); ++i) {
        if (frame.nodes[i].hiddenByCluster) {
            continue;
        }
        const Vector2 anchor = worldToScreen(frame.nodes[i].anchorPosition, screenWidth, screenHeight, camera);
        working.push_back({
            .layoutIndex = i,
            .anchor = anchor,
            .display = anchor,
        });
    }

    constexpr float minDistance = 82.0f;
    constexpr float maxAnchorOffset = 118.0f;
    constexpr int iterations = 14;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        for (std::size_t i = 0; i < working.size(); ++i) {
            for (std::size_t j = i + 1; j < working.size(); ++j) {
                Vector2 delta = subtract(working[j].display, working[i].display);
                float dist = std::max(0.001f, length(delta));
                if (dist >= minDistance) {
                    continue;
                }
                if (dist < 1.0f) {
                    const float angle = static_cast<float>((i * 37 + j * 53) % 360) * 0.017453292f;
                    delta = {std::cos(angle), std::sin(angle)};
                    dist = 1.0f;
                }
                const Vector2 normal = multiply(delta, 1.0f / dist);
                const float push = (minDistance - dist) * 0.5f;
                working[i].display = subtract(working[i].display, multiply(normal, push));
                working[j].display = add(working[j].display, multiply(normal, push));
            }
        }

        for (auto& node : working) {
            const Vector2 toAnchor = subtract(node.anchor, node.display);
            node.display = add(node.display, multiply(toAnchor, 0.18f));
            const Vector2 offset = subtract(node.display, node.anchor);
            const float offsetLength = length(offset);
            if (offsetLength > maxAnchorOffset) {
                node.display = add(node.anchor, multiply(offset, maxAnchorOffset / offsetLength));
            }
        }
    }

    for (const auto& node : working) {
        auto& layout = frame.nodes[node.layoutIndex];
        const Vector2 offset = subtract(node.display, node.anchor);
        if (length(offset) <= 6.0f) {
            continue;
        }
        const Vec2 worldOffset = screenDeltaToWorld(offset, camera);
        layout.displayPosition = {layout.anchorPosition.x + worldOffset.x, layout.anchorPosition.y + worldOffset.y};
        layout.hasOffset = true;
    }
}

GeoLabelLayout GeoLayoutSystem::placeLabel(const Node& node, Vec2 displayPosition, const UiState& state, const CameraController& camera, int screenWidth, int screenHeight, std::vector<Rectangle>& occupiedLabels) const
{
    const Vector2 center = worldToScreen(displayPosition, screenWidth, screenHeight, camera);
    const char* text = node.name.c_str();
    const float labelWidth = static_cast<float>(MeasureText(text, 16));
    const std::array<Vector2, 4> candidates{{
        {center.x - labelWidth * 0.5f, center.y - 74.0f},
        {center.x + 62.0f, center.y - 8.0f},
        {center.x - labelWidth * 0.5f, center.y + 58.0f},
        {center.x - labelWidth - 62.0f, center.y - 8.0f},
    }};

    const bool selected = state.selection.nodeId == node.id;
    for (const Vector2 candidate : candidates) {
        const Rectangle rect = labelRect(candidate, text);
        const bool inBounds = rect.x >= 4.0f && rect.y >= 4.0f
            && rect.x + rect.width <= static_cast<float>(screenWidth - 4)
            && rect.y + rect.height <= static_cast<float>(screenHeight - 4);
        if ((selected || inBounds) && !overlapsAny(rect, occupiedLabels)) {
            occupiedLabels.push_back(rect);
            return {true, screenToWorld(candidate, camera, screenWidth, screenHeight)};
        }
    }

    if (selected) {
        const Vector2 fallback{std::clamp(center.x + 58.0f, 8.0f, static_cast<float>(screenWidth - 140)), std::clamp(center.y - 8.0f, 8.0f, static_cast<float>(screenHeight - 28))};
        occupiedLabels.push_back(labelRect(fallback, text));
        return {true, screenToWorld(fallback, camera, screenWidth, screenHeight)};
    }

    return {};
}
