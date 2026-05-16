#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

enum class SimulationLayer {
    Topology,
    Flow,
    Processing,
    Resource,
    Persistence,
    Coordination,
    Reliability,
    Observability,
    Complexity,
    Geographic,
    EconomicEnergy,
    Evolution,
    Scenario,
    Count
};

struct LayerDebugColor {
    std::uint8_t r = 140;
    std::uint8_t g = 150;
    std::uint8_t b = 165;
    std::uint8_t a = 255;
};

struct LayerDefinition {
    SimulationLayer layer = SimulationLayer::Topology;
    std::string_view displayName;
    std::string_view description;
    bool enabledByDefault = true;
    std::string_view debugIcon;
    LayerDebugColor debugColor{};
};

class LayerRegistry {
public:
    [[nodiscard]] static const LayerDefinition& definition(SimulationLayer layer);
    [[nodiscard]] static std::span<const LayerDefinition> definitions();
    [[nodiscard]] static constexpr std::size_t layerCount()
    {
        return static_cast<std::size_t>(SimulationLayer::Count);
    }
};
