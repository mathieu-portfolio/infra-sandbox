#include "ui/ActionPanelModel.hpp"

#include <array>

namespace {
std::string selectedTarget(const Simulation& simulation, const UiState& state)
{
    if (const Node* node = simulation.graph().node(state.selection.nodeId)) {
        return node->name;
    }
    return "Global";
}

ActionCard mechanicCard(const Simulation& simulation, const UiState& state, MechanicType mechanic, std::string name, std::string description, std::string helps, std::string tradeOff)
{
    const bool allowed = simulation.isMechanicAllowed(mechanic);
    return {
        .kind = ActionCardKind::Mechanic,
        .mechanic = mechanic,
        .name = std::move(name),
        .description = std::move(description),
        .target = selectedTarget(simulation, state),
        .helps = std::move(helps),
        .tradeOff = std::move(tradeOff),
        .unavailableReason = allowed ? "" : "Locked by scenario progression.",
        .available = allowed,
        .requiresConfirmation = false,
    };
}

ActionCard topologyCard(const Simulation& simulation, const UiState& state, TopologyMutationType mutation, MechanicType mechanic, std::string name, std::string description, std::string helps, std::string tradeOff)
{
    const bool allowed = simulation.isMechanicAllowed(mechanic);
    return {
        .kind = ActionCardKind::TopologyMutation,
        .mechanic = mechanic,
        .mutation = mutation,
        .name = std::move(name),
        .description = std::move(description),
        .target = selectedTarget(simulation, state),
        .helps = std::move(helps),
        .tradeOff = std::move(tradeOff),
        .unavailableReason = allowed ? "" : "Locked by scenario progression.",
        .available = allowed,
        .requiresConfirmation = true,
    };
}
}

Rectangle ActionPanelModel::panelBounds(int screenWidth, int screenHeight) const
{
    return {static_cast<float>(screenWidth - 380), 252.0f, 368.0f, static_cast<float>(screenHeight - 274)};
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
        cards.push_back(mechanicCard(simulation, state, MechanicType::ScaleUp, "Scale Up", "Increase API service capacity.", "Processing capacity, API queue pressure", "May not help downstream DB bottlenecks."));
        cards.push_back(topologyCard(simulation, state, TopologyMutationType::AddCache, MechanicType::AddCache, "Add Cache", "Insert cache on a constrained data path.", "Repeated reads, DB pressure, latency", "Adds state and invalidation complexity."));
        cards.push_back(topologyCard(simulation, state, TopologyMutationType::AddReadReplica, MechanicType::AddReadReplica, "Add Replica", "Add read capacity near a chosen region.", "Read throughput, persistence pressure", "Replication lag and operational complexity."));
        cards.push_back(topologyCard(simulation, state, TopologyMutationType::AddQueue, MechanicType::AddQueue, "Add Queue", "Insert buffering between API and database.", "Burst absorption, retry amplification", "Adds delay and async complexity."));
        cards.push_back(topologyCard(simulation, state, TopologyMutationType::AddRegionalCache, MechanicType::AddRegionalCache, "Regional Cache", "Deploy cache capacity near regional demand.", "Traffic localization, inter-region latency", "More distributed state."));
        cards.push_back(mechanicCard(simulation, state, MechanicType::ToggleRetries, "Toggle Retries", "Enable or disable request retries.", "Retry amplification control", "May increase visible errors when disabled."));
        cards.push_back(mechanicCard(simulation, state, MechanicType::ClearCache, "Clear Cache", "Evict current cache contents.", "Tests cache dependency and recovery", "Temporarily increases DB pressure."));
    }

    const Rectangle panel = panelBounds(screenWidth, screenHeight);
    float y = panel.y + 38.0f;
    for (auto& card : cards) {
        card.bounds = {panel.x + 12.0f, y, panel.width - 24.0f, 66.0f};
        y += 70.0f;
    }
    return cards;
}
