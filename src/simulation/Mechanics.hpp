#pragma once

#include "simulation/SimulationLayer.hpp"

#include <array>
#include <span>
#include <string_view>

class Simulation;

enum class MechanicType {
    ScaleUp,
    ScaleOut,
    EnableCache,
    ClearCache,
    ToggleRetries,
    AdjustRetryPolicy,
    AddCache,
    AddQueue,
    AddLoadBalancer,
    AddReadReplica,
    AddRegionalCache,
    ThrottleTraffic,
    SplitService,
    EnableTracing,
    Count
};

enum class MechanicTargetType {
    Global,
    Node,
    Link
};

struct MechanicDefinition {
    MechanicType type = MechanicType::ScaleUp;
    std::string_view displayName;
    std::string_view description;
    MechanicTargetType targetType = MechanicTargetType::Global;
    std::array<SimulationLayer, 4> affectedLayers{};
    std::size_t affectedLayerCount = 0;
    bool available = false;
};

struct MechanicCommand {
    MechanicType type = MechanicType::ScaleUp;
    int targetId = -1;
    double amount = 0.0;
};

class MechanicRegistry {
public:
    [[nodiscard]] static const MechanicDefinition& definition(MechanicType type);
    [[nodiscard]] static std::span<const MechanicDefinition> definitions();
};

class MechanicExecutor {
public:
    void execute(Simulation& simulation, const MechanicCommand& command) const;
};
