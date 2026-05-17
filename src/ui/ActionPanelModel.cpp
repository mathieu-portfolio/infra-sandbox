#include "ui/ActionPanelModel.hpp"

#include "content/ContentRegistry.hpp"
#include "ui/UiLayout.hpp"

namespace {
std::string selectedTarget(const Simulation& simulation, const UiState& state)
{
    if (const Node* node = simulation.graph().node(state.selection.nodeId)) {
        return node->name;
    }
    return "Global";
}

ActionCard mechanicCard(const Simulation& simulation, const UiState& state, const content::InterventionDefinition& definition)
{
    const bool allowed = simulation.isMechanicAllowed(definition.mechanic);
    return {
        .kind = ActionCardKind::Mechanic,
        .mechanic = definition.mechanic,
        .name = definition.displayName,
        .description = definition.description,
        .target = selectedTarget(simulation, state),
        .helps = definition.expectedBenefits,
        .tradeOff = definition.tradeoffs,
        .unavailableReason = allowed ? "" : "Locked by scenario progression.",
        .available = allowed,
        .requiresConfirmation = definition.requiresConfirmation,
    };
}

ActionCard topologyCard(const Simulation& simulation, const UiState& state, const content::InterventionDefinition& definition)
{
    const bool allowed = simulation.isMechanicAllowed(definition.mechanic);
    return {
        .kind = ActionCardKind::TopologyMutation,
        .mechanic = definition.mechanic,
        .mutation = definition.mutation,
        .name = definition.displayName,
        .description = definition.description,
        .target = selectedTarget(simulation, state),
        .helps = definition.expectedBenefits,
        .tradeOff = definition.tradeoffs,
        .unavailableReason = allowed ? "" : "Locked by scenario progression.",
        .available = allowed,
        .requiresConfirmation = definition.requiresConfirmation,
    };
}
}

Rectangle ActionPanelModel::panelBounds(int screenWidth, int screenHeight) const
{
    return computeUiLayout(screenWidth, screenHeight).rightSidebar;
}

std::vector<ActionCard> ActionPanelModel::buildCards(const Simulation& simulation, const UiState& state, int screenWidth, int screenHeight) const
{
    std::vector<ActionCard> cards;
    if (state.placementActive) {
        cards.push_back({
            .kind = ActionCardKind::ConfirmPreview,
            .name = "Confirm placement",
            .description = "Apply the previewed topology change.",
            .target = selectedTarget(simulation, state),
            .helps = "Commits the selected architecture change.",
            .tradeOff = "Watch metrics after applying.",
            .available = true,
        });
        cards.push_back({
            .kind = ActionCardKind::CancelPreview,
            .name = "Cancel preview",
            .description = "Exit placement without changing topology.",
            .target = "Preview",
            .helps = "Keeps current architecture unchanged.",
            .tradeOff = "No effect.",
            .available = true,
        });
    } else {
        for (const auto& definition : content::ContentRegistry::instance().interventions()) {
            if (!simulation.isMechanicAllowed(definition.mechanic)) {
                continue;
            }
            if (definition.kind == content::InterventionKind::TopologyMutation) {
                cards.push_back(topologyCard(simulation, state, definition));
            } else {
                cards.push_back(mechanicCard(simulation, state, definition));
            }
        }
    }

    const Rectangle panel = panelBounds(screenWidth, screenHeight);
    const Rectangle target{panel.x + 10.0f, panel.y + 10.0f, panel.width - 20.0f, 198.0f};
    const float buttonY = panel.y + panel.height - 54.0f;
    const float previewY = buttonY - UiTheme::gap - 170.0f;
    const Rectangle actions{panel.x + 10.0f, target.y + target.height + UiTheme::gap, panel.width - 20.0f, previewY - (target.y + target.height + UiTheme::gap) - UiTheme::gap};
    float y = actions.y + 38.0f;
    const float cardHeight = 58.0f;
    const float step = 62.0f;
    for (auto& card : cards) {
        card.bounds = {panel.x + 12.0f, y, panel.width - 24.0f, cardHeight};
        y += step;
    }
    return cards;
}
