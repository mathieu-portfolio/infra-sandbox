#include "ui/actions/ActionPanelInteraction.hpp"

#include "ui/actions/ActionFiltering.hpp"
#include "ui/actions/ActionPanelModel.hpp"
#include "ui/actions/ActionPanelTabs.hpp"
#include "ui/actions/WorldActionOverlay.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/ScrollHandling.hpp"
#include "ui/layout/RightSidebarLayout.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstddef>

void ActionPanelInteraction::update(UiContext& context, const UiFrameView& view) const
{
    if (context.state == nullptr) {
        return;
    }
    context.state->hoveredActionIndex = -1;
    context.state->hoveredActionEngineeringCosts.clear();
    const ActionPanelModel model;
    const auto cards = model.buildCards(view, *context.state, context.screenWidth, context.screenHeight);
    const ActionSectionsLayout actionsLayout = model.actionSectionsLayout(*context.state, context.screenWidth, context.screenHeight);
    const Vector2 mouse = GetMousePosition();
    float actionContentBottom = actionsLayout.actionList.y;
    for (const auto& card : cards) {
        if (card.bounds.height > 0.0f) {
            actionContentBottom = std::max(actionContentBottom, card.bounds.y + card.bounds.height + context.state->nodeActionScrollOffset);
        }
    }
    const float actionContentHeight = std::max(0.0f, actionContentBottom - actionsLayout.actionList.y);
    (void)ui::updateScrollOffset(actionsLayout.actionList, actionContentHeight, GetMouseWheelMove(), mouse, context.state->nodeActionScrollOffset);
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight, context.state->dockLayout);
        const RightSidebarLayout panel = computeRightSidebarLayout(layout.rightSidebar);
        const auto categoryLabels = actions_ui::categoryFilterLabels(cards);
        for (int i = 0; i < 4 && i < static_cast<int>(categoryLabels.size()); ++i) {
            if (CheckCollisionPointRec(mouse, actionsLayout.filters[i])) {
                context.state->activeActionCategoryIndex = i;
                context.state->nodeActionScrollOffset = 0.0f;
                context.state->selectedActionIndex = -1;
                return;
            }
        }
        constexpr int kNodeTabCount = static_cast<int>(NodeInspectionTab::Count);
        if (CheckCollisionPointRec(mouse, panel.tabs)) {
            for (int i = 0; i < kNodeTabCount; ++i) {
                if (CheckCollisionPointRec(mouse, tabBounds(panel.tabs, i, kNodeTabCount))) {
                    context.state->activeNodeInspectionTab = nodeTabAt(i);
                    return;
                }
            }
        }
    }
    context.state->hoveredWorldActionIndex = -1;
    if (context.state->eventPopupMode != EventPopupMode::None) {
        return;
    }
    if (context.state->gameplayPhase == GameplayPhase::Planning && context.state->worldActionDraftVisible) {
        const Rectangle overlay = WorldActionOverlay::overlayBounds(context.screenWidth, context.screenHeight, context.state->worldActionDraft);
        const int count = static_cast<int>(context.state->worldActionDraft.size());
        for (int i = 0; i < static_cast<int>(context.state->worldActionDraft.size()); ++i) {
            if (CheckCollisionPointRec(mouse, WorldActionOverlay::draftCardBounds(overlay, i, count))) {
                context.state->hoveredWorldActionIndex = i;
                return;
            }
        }
    }
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        if (CheckCollisionPointRec(mouse, cards[static_cast<std::size_t>(i)].bounds)) {
            context.state->hoveredActionIndex = i;
            context.state->hoveredActionEngineeringCosts = cards[static_cast<std::size_t>(i)].engineeringCosts;
            return;
        }
    }
}
