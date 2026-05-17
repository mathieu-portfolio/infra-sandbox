#include "rendering/VisualFeedbackSystem.hpp"

#include <algorithm>
#include <cmath>

void VisualFeedbackSystem::submit(const VisualFeedbackEvent& event)
{
    switch (event.kind) {
    case VisualFeedbackKind::ActionAcknowledged:
        effects_.push_back({
            .type = VisualEffectType::NodePulse,
            .targetNodeId = event.targetNodeId,
            .targetLinkId = event.targetLinkId,
            .mutation = event.mutation,
            .durationSeconds = 1.1,
            .strength = 0.85f,
        });
        break;
    case VisualFeedbackKind::TopologyMutation:
        effects_.push_back({
            .type = VisualEffectType::TopologyMaterialize,
            .targetNodeId = event.targetNodeId,
            .targetLinkId = event.targetLinkId,
            .mutation = event.mutation,
            .durationSeconds = 2.2,
            .strength = 1.0f,
        });
        effects_.push_back({
            .type = VisualEffectType::TrafficReroute,
            .targetNodeId = event.targetNodeId,
            .targetLinkId = event.targetLinkId,
            .mutation = event.mutation,
            .durationSeconds = 5.5,
            .strength = 0.8f,
        });
        break;
    case VisualFeedbackKind::TrafficShift:
    case VisualFeedbackKind::PressureInjected:
        effects_.push_back({
            .type = VisualEffectType::PressurePulse,
            .targetNodeId = event.targetNodeId,
            .targetLinkId = event.targetLinkId,
            .durationSeconds = 2.4,
            .strength = 0.9f,
        });
        break;
    case VisualFeedbackKind::Stabilization:
        effects_.push_back({
            .type = VisualEffectType::Stabilization,
            .targetNodeId = event.targetNodeId,
            .targetLinkId = event.targetLinkId,
            .durationSeconds = 3.0,
            .strength = 0.7f,
        });
        break;
    }
}

void VisualFeedbackSystem::update(double dt, const Simulation& simulation)
{
    routeShift_ = 0.0f;
    deploymentRipple_ = 0.0f;
    for (auto& effect : effects_) {
        effect.ageSeconds += dt;
        const float intensity = effectIntensity(effect);
        if (effect.type == VisualEffectType::TrafficReroute) {
            routeShift_ = std::max(routeShift_, intensity);
        }
        if (effect.type == VisualEffectType::TopologyMaterialize) {
            deploymentRipple_ = std::max(deploymentRipple_, intensity);
        }
    }
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(), [](const VisualEffect& effect) {
            return effect.ageSeconds >= effect.durationSeconds;
        }),
        effects_.end());
    updatePressureState(dt, simulation);
}

float VisualFeedbackSystem::nodeActionPulse(int nodeId) const
{
    float value = 0.0f;
    for (const auto& effect : effects_) {
        if (effect.type == VisualEffectType::NodePulse && (effect.targetNodeId == nodeId || effect.targetNodeId < 0)) {
            value = std::max(value, effectIntensity(effect));
        }
        if (effect.type == VisualEffectType::TopologyMaterialize && effect.targetNodeId < 0) {
            value = std::max(value, effectIntensity(effect) * 0.45f);
        }
    }
    return value;
}

float VisualFeedbackSystem::nodePressureGlow(int nodeId) const
{
    if (const auto it = pressureByNode_.find(nodeId); it != pressureByNode_.end()) {
        return it->second;
    }
    return 0.0f;
}

float VisualFeedbackSystem::nodeInstability(int nodeId) const
{
    if (const auto it = instabilityByNode_.find(nodeId); it != instabilityByNode_.end()) {
        return it->second;
    }
    return 0.0f;
}

float VisualFeedbackSystem::linkActivation(int linkId) const
{
    float value = 0.0f;
    for (const auto& effect : effects_) {
        if (effect.type == VisualEffectType::TopologyMaterialize && (effect.targetLinkId == linkId || effect.targetLinkId < 0)) {
            value = std::max(value, effectIntensity(effect));
        }
    }
    return value;
}

float VisualFeedbackSystem::linkThroughputBoost(int linkId) const
{
    const float live = linkIntensity_.contains(linkId) ? linkIntensity_.at(linkId) : 0.0f;
    return std::max(live, routeShift_ * 0.55f);
}

float VisualFeedbackSystem::routeShift() const
{
    return routeShift_;
}

float VisualFeedbackSystem::deploymentRipple() const
{
    return deploymentRipple_;
}

float VisualFeedbackSystem::effectIntensity(const VisualEffect& effect) const
{
    const float t = effect.durationSeconds > 0.0 ? static_cast<float>(effect.ageSeconds / effect.durationSeconds) : 1.0f;
    const float envelope = std::clamp(1.0f - t, 0.0f, 1.0f);
    const float ease = envelope * envelope * (3.0f - 2.0f * envelope);
    return ease * effect.strength;
}

void VisualFeedbackSystem::updatePressureState(double dt, const Simulation& simulation)
{
    std::unordered_map<int, float> targets;
    std::unordered_map<int, float> instabilityTargets;
    for (const auto& pressure : simulation.pressure().nodes) {
        const float pressureValue = static_cast<float>(std::clamp(
            pressure.queuePressure * 0.35 + pressure.computePressure * 0.25 + pressure.latencyContribution * 0.2 + pressure.retryContribution * 0.2,
            0.0,
            1.0));
        targets[pressure.nodeId] = pressureValue;
        instabilityTargets[pressure.nodeId] = static_cast<float>(std::clamp(pressure.instability, 0.0, 1.0));
    }

    const float alpha = std::clamp(static_cast<float>(dt * 4.5), 0.0f, 1.0f);
    for (const auto& [nodeId, target] : targets) {
        pressureByNode_[nodeId] += (target - pressureByNode_[nodeId]) * alpha;
    }
    for (auto& [nodeId, value] : pressureByNode_) {
        if (!targets.contains(nodeId)) {
            value += (0.0f - value) * alpha;
        }
    }
    for (const auto& [nodeId, target] : instabilityTargets) {
        instabilityByNode_[nodeId] += (target - instabilityByNode_[nodeId]) * alpha;
    }
    for (auto& [nodeId, value] : instabilityByNode_) {
        if (!instabilityTargets.contains(nodeId)) {
            value += (0.0f - value) * alpha;
        }
    }

    for (const auto& link : simulation.graph().links()) {
        const float target = static_cast<float>(std::clamp(static_cast<double>(link.inFlightRequests.size()) / 18.0, 0.0, 1.0));
        linkIntensity_[link.id] += (target - linkIntensity_[link.id]) * alpha;
    }
    for (auto& [linkId, value] : linkIntensity_) {
        const auto exists = std::find_if(simulation.graph().links().begin(), simulation.graph().links().end(), [linkId](const Link& link) {
            return link.id == linkId;
        }) != simulation.graph().links().end();
        if (!exists) {
            value += (0.0f - value) * alpha;
        }
    }
}
