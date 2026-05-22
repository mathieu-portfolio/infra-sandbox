#pragma once

#include "rendering/GeoLayoutSystem.hpp"
#include "rendering/TopologyPresentationState.hpp"
#include "simulation/core/Simulation.hpp"
#include "ui/core/UiTypes.hpp"

namespace rendering::viewmodels {

// Frame-level view model shared by renderer layers.
// It is intentionally thin for now: it centralizes the data each layer needs,
// so future passes can replace direct Simulation access with prepared node/link/request DTOs incrementally.
struct RenderFrameView {
    const Simulation& simulation;
    const UiState& uiState;
    const GeoLayoutFrame& geoLayout;
    const rendering::TopologyPresentationState& presentation;
    int screenWidth = 0;
    int screenHeight = 0;
};

} // namespace rendering::viewmodels
