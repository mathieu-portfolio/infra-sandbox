#include "ui/InterventionPanel.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>

void InterventionPanel::update(UiContext&, const Simulation&)
{
}

void InterventionPanel::draw(const UiContext& context, const Simulation& simulation) const
{
    if (context.state == nullptr || !context.state->placementActive) {
        return;
    }

    const PlacementCandidateGenerator generator;
    const MutationValidator validator;
    const auto candidates = generator.generate(simulation, context.state->activeMutation);
    const int x = context.screenWidth - 344;
    const int y = 282;
    DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y), 332.0f, 196.0f}, 0.04f, 8, {22, 27, 34, 235});
    DrawText(topologyMutationName(context.state->activeMutation), x + 12, y + 10, 18, {230, 237, 243, 255});

    if (candidates.empty()) {
        DrawText("No valid placement candidates", x + 12, y + 42, 16, {235, 86, 100, 255});
        return;
    }

    const int index = std::clamp(context.state->placementCandidateIndex, 0, static_cast<int>(candidates.size()) - 1);
    const auto& option = candidates[static_cast<std::size_t>(index)];
    const MutationPreview preview = validator.preview(simulation, context.state->activeMutation, option);

    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "%d/%zu  %s", index + 1, candidates.size(), option.displayName.c_str());
    DrawText(buffer, x + 12, y + 40, 16, {89, 196, 255, 255});
    DrawText(option.latencyImpact.c_str(), x + 12, y + 66, 15, {230, 237, 243, 255});
    DrawText(option.trafficImpact.c_str(), x + 12, y + 90, 15, {139, 148, 158, 255});
    std::snprintf(buffer, sizeof(buffer), "Resource: %s", option.resourceCost.c_str());
    DrawText(buffer, x + 12, y + 116, 15, {139, 148, 158, 255});
    std::snprintf(buffer, sizeof(buffer), "Complexity: %s", option.complexityImpact.c_str());
    DrawText(buffer, x + 12, y + 140, 15, {139, 148, 158, 255});
    DrawText(preview.validationMessage.c_str(), x + 12, y + 164, 15, preview.valid ? Color{86, 210, 151, 255} : Color{235, 86, 100, 255});
    DrawText("Left/Right choose | Enter confirm | Backspace cancel", x + 12, y + 180, 13, {139, 148, 158, 255});
}
