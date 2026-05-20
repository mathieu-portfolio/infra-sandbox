#include "simulation/core/SimulationConfig.hpp"

SimulationConfig::SimulationConfig()
{
    for (const auto& definition : LayerRegistry::definitions()) {
        const auto index = static_cast<std::size_t>(definition.layer);
        layers[index] = {
            .layer = definition.layer,
            .enabled = definition.enabledByDefault,
        };
    }
}

bool SimulationConfig::isLayerEnabled(SimulationLayer layer) const
{
    const auto index = static_cast<std::size_t>(layer);
    if (index >= layers.size()) {
        return false;
    }
    return layers[index].enabled;
}

void SimulationConfig::setLayerEnabled(SimulationLayer layer, bool enabled)
{
    const auto index = static_cast<std::size_t>(layer);
    if (index >= layers.size()) {
        return;
    }
    layers[index].enabled = enabled;
}
