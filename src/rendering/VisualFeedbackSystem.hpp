#pragma once

#include "simulation/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

#include <unordered_map>
#include <vector>

enum class VisualEffectType {
    NodePulse,
    TopologyMaterialize,
    TrafficReroute,
    PressurePulse,
    Stabilization
};

struct VisualEffect {
    VisualEffectType type = VisualEffectType::NodePulse;
    int targetNodeId = -1;
    int targetLinkId = -1;
    TopologyMutationType mutation = TopologyMutationType::AddCache;
    double ageSeconds = 0.0;
    double durationSeconds = 1.0;
    float strength = 1.0f;
};

struct VisualFeedbackEventRecord {
    VisualFeedbackKind kind = VisualFeedbackKind::ActionAcknowledged;
    int targetNodeId = -1;
    int targetLinkId = -1;
    MechanicType mechanic = MechanicType::ScaleUp;
    TopologyMutationType mutation = TopologyMutationType::AddCache;
};

class VisualFeedbackSystem {
public:
    void submit(const VisualFeedbackEvent& event);
    void update(double dt, const Simulation& simulation);

    [[nodiscard]] float nodeActionPulse(int nodeId) const;
    [[nodiscard]] float nodePressureGlow(int nodeId) const;
    [[nodiscard]] float nodeInstability(int nodeId) const;
    [[nodiscard]] float linkActivation(int linkId) const;
    [[nodiscard]] float linkThroughputBoost(int linkId) const;
    [[nodiscard]] float routeShift() const;
    [[nodiscard]] float deploymentRipple() const;

private:
    [[nodiscard]] float effectIntensity(const VisualEffect& effect) const;
    void updatePressureState(double dt, const Simulation& simulation);

    std::vector<VisualEffect> effects_;
    std::unordered_map<int, float> pressureByNode_;
    std::unordered_map<int, float> instabilityByNode_;
    std::unordered_map<int, float> linkIntensity_;
    float routeShift_ = 0.0f;
    float deploymentRipple_ = 0.0f;
};
