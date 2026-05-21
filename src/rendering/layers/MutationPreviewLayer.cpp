#include "rendering/Renderer.hpp"

#include "rendering/layers/RendererLayerUtils.hpp"
#include "ui/widgets/IconRegistry.hpp"
#include "ui/viewmodels/UiFrameView.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace rendering::layers;

namespace {
UiFrameView buildSimulationOnlyUiFrameView(const Simulation& simulation)
{
    UiFrameView view{};
    view.graphValue = &simulation.graph();
    view.requestsValue = &simulation.requests();
    view.runtimeSystemsValue = &simulation.runtimeSystems();
    view.metricsValue = simulation.metrics();
    view.pressureValue = simulation.pressure();
    view.timeSecondsValue = simulation.timeSeconds();
    view.simulationSpeedValue = simulation.simulationSpeed();
    view.mechanicAllowed = [&simulation](MechanicType mechanic) { return simulation.isMechanicAllowed(mechanic); };
    view.canScaleNodeValue = [&simulation](int nodeId, int maxScaleLevel) { return simulation.canScaleNode(nodeId, maxScaleLevel); };
    view.scaleLevelForNodeValue = [&simulation](int nodeId) { return simulation.scaleLevelForNode(nodeId); };
    view.maxScaleLevelForNodeValue = [&simulation](int nodeId, int contentMaxScaleLevel) { return simulation.maxScaleLevelForNode(nodeId, contentMaxScaleLevel); };
    view.hasAnyRegionCapacityValue = [&simulation](int slots) { return simulation.hasAnyRegionCapacity(slots); };
    return view;
}
}

void Renderer::drawMutationPreview(const rendering::viewmodels::RenderFrameView& frame, const CameraController& camera) const
{
    const Simulation& simulation = frame.simulation;

    const UiState& state = frame.uiState;
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    const PlacementCandidateGenerator generator;
    const MutationValidator validator;
    TopologyMutationType mutation = state.activeMutation;
    bool activePreview = state.placementActive;
    if (!activePreview) {
        const ActionPanelModel model;
        const UiFrameView view = buildSimulationOnlyUiFrameView(simulation);
        const auto cards = model.buildCards(view, state, width, height);
        const int index = state.hoveredActionIndex >= 0 ? state.hoveredActionIndex : state.selectedActionIndex;
        if (index >= 0 && index < static_cast<int>(cards.size()) && cards[static_cast<std::size_t>(index)].kind == ActionCardKind::TopologyMutation && cards[static_cast<std::size_t>(index)].available) {
            mutation = cards[static_cast<std::size_t>(index)].mutation;
            activePreview = true;
        }
    }
    if (!activePreview) {
        return;
    }

    const auto candidates = generator.generate(simulation, mutation);
    if (candidates.empty()) {
        return;
    }

    int selected = 0;
    if (state.placementActive) {
        selected = hoveredPlacementCandidateIndex(simulation, mutation, camera, GetMousePosition(), width, height);
    }
    for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
        const Vec2 world = MapProjection::projectEquirectangular(candidates[static_cast<std::size_t>(i)].location);
        const Vector2 center = worldToScreen(world, width, height, camera);
        const bool isSelected = selected >= 0 && i == selected;
        const Color color = isSelected ? Color{89, 196, 255, 210} : Color{139, 148, 158, 120};
        DrawCircleV(center, isSelected ? 38.0f : 28.0f, {color.r, color.g, color.b, 34});
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), isSelected ? 38.0f : 28.0f, color);
        DrawText(candidates[static_cast<std::size_t>(i)].displayName.c_str(), static_cast<int>(center.x + 42.0f), static_cast<int>(center.y - 8.0f), 14, color);
    }

    if (selected < 0 || selected >= static_cast<int>(candidates.size())) {
        return;
    }

    const auto& option = candidates[static_cast<std::size_t>(selected)];
    const MutationPreview preview = validator.preview(simulation, mutation, option);
    const Color ghost{89, 196, 255, 150};
    for (const auto& node : preview.mutation.nodesToCreate) {
        const Vector2 center = worldToScreen(node.position, width, height, camera);
        DrawCircleV(center, 32.0f, {ghost.r, ghost.g, ghost.b, 42});
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 32.0f, ghost);
        DrawText(node.name.c_str(), static_cast<int>(center.x + 38.0f), static_cast<int>(center.y + 18.0f), 14, ghost);
    }

    for (const auto& link : preview.mutation.linksToCreate) {
        Vec2 source{};
        Vec2 target{};
        bool hasSource = false;
        bool hasTarget = false;
        if (const Node* sourceNode = simulation.graph().node(link.sourceNodeId)) {
            source = sourceNode->position;
            hasSource = true;
        } else if (!preview.mutation.nodesToCreate.empty()) {
            source = preview.mutation.nodesToCreate.front().position;
            hasSource = true;
        }
        if (const Node* targetNode = simulation.graph().node(link.targetNodeId)) {
            target = targetNode->position;
            hasTarget = true;
        } else if (!preview.mutation.nodesToCreate.empty()) {
            target = preview.mutation.nodesToCreate.front().position;
            hasTarget = true;
        }
        if (hasSource && hasTarget) {
            drawArc(source, target, width, height, camera, 2.0f, {89, 196, 255, 110});
        }
    }
}
