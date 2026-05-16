#include "ui/OverlayController.hpp"

#include <algorithm>

namespace {
Color tint(Color color, float intensity)
{
    const auto alpha = static_cast<unsigned char>(std::clamp(intensity, 0.0f, 1.0f) * 120.0f);
    return {color.r, color.g, color.b, alpha};
}
}

Color OverlayController::nodeTint(const Node& node, const Simulation& simulation, const UiState& state) const
{
    const NodePressure* pressure = simulation.pressureAnalysis().pressureForNode(node.id);
    switch (state.activeOverlay) {
    case OverlayMode::None:
        return {0, 0, 0, 0};
    case OverlayMode::Flow:
        return tint({89, 196, 255, 255}, node.queue.empty() ? 0.2f : 0.55f);
    case OverlayMode::Latency:
        return tint({245, 184, 76, 255}, static_cast<float>(std::min(1.0, node.averageQueueWaitSeconds / 3.0)));
    case OverlayMode::Utilization:
        return tint({86, 210, 151, 255}, static_cast<float>(node.currentUtilization));
    case OverlayMode::Queues:
        return tint({245, 184, 76, 255}, static_cast<float>(std::min(1.0, static_cast<double>(node.queue.size()) / 12.0)));
    case OverlayMode::Errors:
        return tint({235, 86, 100, 255}, simulation.metrics().timeoutRatePerSecond > 0.0 ? 0.7f : 0.15f);
    case OverlayMode::Reliability:
        return tint({235, 86, 100, 255}, node.health == HealthState::Healthy ? 0.15f : 0.75f);
    case OverlayMode::Complexity:
        return tint({187, 128, 255, 255}, NodeRegistry::routesRequests(node.type) ? 0.45f : 0.2f);
    case OverlayMode::Bottlenecks:
        if (node.id == simulation.pressure().topOverloadedNodeId || node.id == simulation.pressure().mostUnstableNodeId) {
            return tint({255, 214, 102, 255}, 0.9f);
        }
        return tint({255, 214, 102, 255}, pressure != nullptr ? static_cast<float>(pressure->instability * 0.45) : 0.1f);
    case OverlayMode::RetryAmplification:
        if (node.id == simulation.pressure().retryAmplificationNodeId) {
            return tint({235, 86, 100, 255}, 0.9f);
        }
        return tint({235, 86, 100, 255}, pressure != nullptr ? static_cast<float>(pressure->retryContribution) : 0.0f);
    }

    return {0, 0, 0, 0};
}
