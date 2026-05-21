#include "ui/actions/ActionPanelModel.hpp"

#include "content/ContentRegistry.hpp"
#include "ui/actions/cards/NodeActionCardView.hpp"
#include "ui/actions/EngineeringCapacityPanel.hpp"
#include "ui/actions/ActionFiltering.hpp"
#include "ui/core/UiCore.hpp"
#include "ui/layout/RightSidebarLayout.hpp"
#include "ui/core/UiLayout.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>

namespace {
Rectangle nodeBounds(const ui::UiNode& root, const char* id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
}

int capacityForDomain(const EngineeringCapacity& capacity, EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend:
        return capacity.frontend;
    case EngineeringDomain::Backend:
        return capacity.backend;
    case EngineeringDomain::Infrastructure:
        return capacity.infrastructure;
    case EngineeringDomain::Data:
        return capacity.data;
    case EngineeringDomain::Count:
        break;
    }
    return 0;
}

const EngineeringCapacity& effectivePlanningCapacity(const UiState& state)
{
    return state.engineeringCapacityPreviewVisible && state.selectedWorldActionIndex >= 0
        ? state.previewEngineeringCapacity
        : state.engineeringCapacity;
}

std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> plannedDomainUsage(const UiState& state)
{
    std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> usage{};
    for (const auto& planned : state.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            usage[static_cast<std::size_t>(cost.domain)] += cost.amount;
        }
    }
    return usage;
}

bool exceedsEngineeringCapacity(const UiState& state, const std::vector<EngineeringCost>& costs, std::string& reason)
{
    const auto usage = plannedDomainUsage(state);
    const EngineeringCapacity& capacity = effectivePlanningCapacity(state);
    int plannedTotal = 0;
    int addedTotal = 0;
    for (const int used : usage) {
        plannedTotal += used;
    }
    for (const auto& cost : costs) {
        addedTotal += cost.amount;
        const int next = usage[static_cast<std::size_t>(cost.domain)] + cost.amount;
        const int cap = capacityForDomain(capacity, cost.domain);
        if (next > cap) {
            reason = std::string("Insufficient ") + engineeringDomainName(cost.domain) + " capacity this turn.";
            return true;
        }
    }
    if (plannedTotal + addedTotal > capacity.total) {
        reason = "Insufficient turn budget.";
        return true;
    }
    return false;
}

bool targetsSelectedNode(const UiFrameView& view, const UiState& state, const content::InterventionDefinition& definition)
{
    const Node* node = view.graph().node(state.selection.nodeId);
    if (node == nullptr) {
        return false;
    }
    if (definition.targetNodeTypes.empty()) {
        return definition.mechanic == MechanicType::ToggleRetries || definition.mechanic == MechanicType::ThrottleTraffic;
    }
    return std::find(definition.targetNodeTypes.begin(), definition.targetNodeTypes.end(), node->type) != definition.targetNodeTypes.end();
}

bool pressureMatches(const UiFrameView& view, const UiState& state, const content::InterventionDefinition& definition)
{
    const NodePressure* pressure = view.pressureAnalysis().pressureForNode(state.selection.nodeId);
    if (pressure == nullptr || pressure->dominant == PressureCategory::None) {
        return false;
    }
    return std::find(definition.affectedPressures.begin(), definition.affectedPressures.end(), pressure->dominant) != definition.affectedPressures.end();
}

std::string selectedTarget(const UiFrameView& view, const UiState& state)
{
    if (const Node* node = view.graph().node(state.selection.nodeId)) {
        return node->name;
    }
    return "Global";
}

ActionCardModel mechanicCard(const UiFrameView& view, const UiState& state, const content::InterventionDefinition& definition)
{
    bool available = view.isMechanicAllowed(definition.mechanic);
    std::string unavailableReason = available ? "" : "Locked by scenario progression.";
    const bool recommended = pressureMatches(view, state, definition);
    std::string stateLabel = available ? (recommended ? "Suggested" : "Available") : "Locked";
    int currentScaleLevel = 0;
    int maxScaleLevel = 0;
    if (definition.mechanic == MechanicType::ScaleUp) {
        maxScaleLevel = view.maxScaleLevelForNode(state.selection.nodeId, definition.maxScaleLevel);
        currentScaleLevel = view.scaleLevelForNode(state.selection.nodeId);
        if (state.selection.nodeId >= 0) {
            const Node* node = view.graph().node(state.selection.nodeId);
            if (node == nullptr || node->type != NodeType::ApiService) {
                available = false;
                unavailableReason = "Invalid target: select an API service.";
                stateLabel = "Invalid target";
            }
        }
        if (available && !view.canScaleNode(state.selection.nodeId, definition.maxScaleLevel)) {
            available = false;
            unavailableReason = "Max scale level reached.";
            stateLabel = "Maxed";
        }
    }
    const int used = state.actionUseCounts.count(definition.id) ? state.actionUseCounts.at(definition.id) : 0;
    if (available && definition.useLimit > 0 && used >= definition.useLimit) {
        available = false;
        unavailableReason = "No uses remaining for this scenario.";
        stateLabel = "Used";
    }
    std::string capacityReason;
    if (available && exceedsEngineeringCapacity(state, definition.engineeringCosts, capacityReason)) {
        available = false;
        unavailableReason = capacityReason;
        stateLabel = "Capacity";
    }
    return {
        .kind = ActionCardKind::Mechanic,
        .mechanic = definition.mechanic,
        .actionId = definition.id,
        .name = definition.displayName,
        .description = definition.description,
        .target = selectedTarget(view, state),
        .helps = definition.expectedBenefits,
        .tradeOff = definition.tradeoffs,
        .positiveEffects = definition.positiveEffects,
        .negativeEffects = definition.negativeEffects,
        .pressureShifts = definition.pressureShifts,
        .categories = definition.categories,
        .usefulWhen = definition.usefulWhen,
        .showUsageDetails = definition.showUsageDetails,
        .affectedPressures = definition.affectedPressures,
        .engineeringCosts = definition.engineeringCosts,
        .architecturalPattern = definition.architecturalPattern,
        .technologyExample = definition.technologyExample,
        .iconId = definition.iconId,
        .unavailableReason = unavailableReason,
        .stateLabel = stateLabel,
        .recommended = recommended,
        .complexityCost = definition.complexityCost,
        .currentScaleLevel = currentScaleLevel,
        .maxScaleLevel = maxScaleLevel,
        .available = available,
        .useLimit = definition.useLimit,
        .usesRemaining = std::max(0, definition.useLimit - (state.actionUseCounts.count(definition.id) ? state.actionUseCounts.at(definition.id) : 0)),
        .requiresConfirmation = definition.requiresConfirmation,
    };
}

ActionCardModel topologyCard(const UiFrameView& view, const UiState& state, const content::InterventionDefinition& definition)
{
    bool available = view.isMechanicAllowed(definition.mechanic);
    std::string unavailableReason = available ? "" : "Locked by scenario progression.";
    const bool recommended = pressureMatches(view, state, definition);
    std::string stateLabel = available ? (recommended ? "Suggested" : "Available") : "Locked";
    if (available && !view.hasAnyRegionCapacity(definition.regionSlotUsage)) {
        available = false;
        unavailableReason = "Insufficient regional deployment capacity.";
        stateLabel = "No capacity";
    }
    const int used = state.actionUseCounts.count(definition.id) ? state.actionUseCounts.at(definition.id) : 0;
    if (available && definition.useLimit > 0 && used >= definition.useLimit) {
        available = false;
        unavailableReason = "No uses remaining for this scenario.";
        stateLabel = "Used";
    }
    std::string capacityReason;
    if (available && exceedsEngineeringCapacity(state, definition.engineeringCosts, capacityReason)) {
        available = false;
        unavailableReason = capacityReason;
        stateLabel = "Capacity";
    }
    return {
        .kind = ActionCardKind::TopologyMutation,
        .mechanic = definition.mechanic,
        .actionId = definition.id,
        .mutation = definition.mutation,
        .name = definition.displayName,
        .description = definition.description,
        .target = selectedTarget(view, state),
        .helps = definition.expectedBenefits,
        .tradeOff = definition.tradeoffs,
        .positiveEffects = definition.positiveEffects,
        .negativeEffects = definition.negativeEffects,
        .pressureShifts = definition.pressureShifts,
        .categories = definition.categories,
        .usefulWhen = definition.usefulWhen,
        .showUsageDetails = definition.showUsageDetails,
        .affectedPressures = definition.affectedPressures,
        .engineeringCosts = definition.engineeringCosts,
        .architecturalPattern = definition.architecturalPattern,
        .technologyExample = definition.technologyExample,
        .iconId = definition.iconId,
        .unavailableReason = unavailableReason,
        .stateLabel = stateLabel,
        .recommended = recommended,
        .complexityCost = definition.complexityCost,
        .regionSlotUsage = definition.regionSlotUsage,
        .available = available,
        .useLimit = definition.useLimit,
        .usesRemaining = std::max(0, definition.useLimit - (state.actionUseCounts.count(definition.id) ? state.actionUseCounts.at(definition.id) : 0)),
        .requiresConfirmation = definition.requiresConfirmation,
    };
}
}

Rectangle ActionPanelModel::panelBounds(int screenWidth, int screenHeight) const
{
    return computeUiLayout(screenWidth, screenHeight).rightSidebar;
}


ActionSectionsLayout ActionPanelModel::actionSectionsLayout(const UiState& state, int screenWidth, int screenHeight) const
{
    const Rectangle panel = panelBounds(screenWidth, screenHeight);
    const RightSidebarLayout layout = computeRightSidebarLayout(panel);

    const float top = layout.actionHeader.y;
    const float bottom = layout.actionList.y + layout.actionList.height;
    const Rectangle actionArea{layout.actionHeader.x, top, layout.actionHeader.width, std::max(0.0f, bottom - top)};

    auto root = ui::verticalStack("actionSections");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = 8.0f;
    root->style(rootStyle);

    auto title = std::make_unique<ui::PanelNode>("title");
    title->style(ui::fixedHeight(18.0f));
    root->add(std::move(title));

    auto capacity = std::make_unique<ui::PanelNode>("capacity");
    capacity->style(ui::fixedHeight(EngineeringCapacityPanel{}.measureHeight(state)));
    root->add(std::move(capacity));

    auto filters = ui::grid(4, "filters");
    ui::LayoutStyle filtersStyle;
    filtersStyle.heightMode = ui::SizeMode::Fixed;
    filtersStyle.fixedHeight = 26.0f;
    filtersStyle.widthMode = ui::SizeMode::Flex;
    filtersStyle.flexGrow = 1.0f;
    filters->style(filtersStyle);
    filters->columnGap = 10.0f;
    filters->fixedCellHeight = 26.0f;
    for (int i = 0; i < 4; ++i) {
        auto filter = std::make_unique<ui::PanelNode>("filter" + std::to_string(i));
        filter->style(ui::fixedHeight(26.0f));
        filters->add(std::move(filter));
    }
    root->add(std::move(filters));

    auto cards = std::make_unique<ui::PanelNode>("cards");
    ui::LayoutStyle cardsStyle;
    cardsStyle.heightMode = ui::SizeMode::Flex;
    cardsStyle.flexGrow = 1.0f;
    cards->style(cardsStyle);
    root->add(std::move(cards));

    root->measure({actionArea.width, actionArea.height});
    root->layout(actionArea);

    ActionSectionsLayout result;
    result.title = nodeBounds(*root, "title");
    result.capacity = nodeBounds(*root, "capacity");
    result.actionList = nodeBounds(*root, "cards");
    for (int i = 0; i < 4; ++i) {
        result.filters[i] = nodeBounds(*root, ("filter" + std::to_string(i)).c_str());
    }
    return result;
}

std::vector<ActionCardModel> ActionPanelModel::buildCards(const UiFrameView& view, const UiState& state, int screenWidth, int screenHeight) const
{
    std::vector<ActionCardModel> cards;
    if (state.placementActive) {
        cards.push_back({
            .kind = ActionCardKind::ConfirmPreview,
            .name = "Click map to place",
            .description = "Hover a continent to preview, then click the map to queue placement.",
            .target = selectedTarget(view, state),
            .helps = "Places the selected node directly on the hovered region.",
            .tradeOff = "Consequences resolve during the turn.",
            .showUsageDetails = false,
            .available = true,
        });
        cards.push_back({
            .kind = ActionCardKind::CancelPreview,
            .name = "Cancel preview",
            .description = "Exit placement without changing topology.",
            .target = "Preview",
            .helps = "Keeps current architecture unchanged.",
            .tradeOff = "No effect.",
            .showUsageDetails = false,
            .available = true,
        });
    } else {
        for (const auto& definition : content::ContentRegistry::instance().interventions()) {
            if (!targetsSelectedNode(view, state, definition)) {
                continue;
            }
            if (definition.kind == content::InterventionKind::TopologyMutation) {
                cards.push_back(topologyCard(view, state, definition));
            } else {
                cards.push_back(mechanicCard(view, state, definition));
            }
        }
        std::stable_sort(cards.begin(), cards.end(), [](const ActionCardModel& lhs, const ActionCardModel& rhs) {
            return lhs.recommended && !rhs.recommended;
        });
    }

    const ActionSectionsLayout layout = actionSectionsLayout(state, screenWidth, screenHeight);
    const Rectangle list = layout.actionList;
    const auto labels = actions_ui::categoryFilterLabels(cards);
    const int activeCategory = std::clamp(state.activeActionCategoryIndex, 0, static_cast<int>(labels.size()) - 1);
    const int columns = list.width >= 524.0f ? 2 : 1;
    const float cardGap = 16.0f;
    const float cardWidth = columns > 1 ? (list.width - cardGap) * 0.5f : list.width;
    float y = list.y - std::max(0.0f, state.nodeActionScrollOffset);
    const NodeActionCardView cardView;

    for (std::size_t i = 0; i < cards.size();) {
        std::vector<std::size_t> row;
        row.reserve(static_cast<std::size_t>(columns));
        while (i < cards.size() && static_cast<int>(row.size()) < columns) {
            auto& card = cards[i];
            if (!card.available || !actions_ui::actionMatchesCategory(card, labels, activeCategory)) {
                card.bounds = {};
            } else {
                row.push_back(i);
            }
            ++i;
        }

        if (row.empty()) {
            continue;
        }

        float rowHeight = 0.0f;
        for (const std::size_t cardIndex : row) {
            rowHeight = std::max(rowHeight, cardView.measureHeight(cards[cardIndex], cardWidth));
        }

        for (std::size_t column = 0; column < row.size(); ++column) {
            const float x = list.x + static_cast<float>(column) * (cardWidth + cardGap);
            cards[row[column]].bounds = {x, y, cardWidth, rowHeight};
        }
        y += rowHeight + cardGap;
    }
    return cards;
}
