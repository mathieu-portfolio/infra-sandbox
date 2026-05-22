#include "ui/actions/WorldActionOverlay.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiCore.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/UiPrimitives.hpp"
#include "ui/actions/cards/WorldActionCardView.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>


namespace {
Rectangle nodeBounds(const ui::UiNode& root, const char* id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
}

Rectangle draftCardsArea(Rectangle overlay)
{
    constexpr float kOverlayPadding = 20.0f;
    constexpr float kHeaderHeight = 66.0f;
    constexpr float kBottomPadding = 30.0f;

    return {
        overlay.x + kOverlayPadding,
        overlay.y + kHeaderHeight + kOverlayPadding,
        overlay.width - kOverlayPadding * 2.0f,
        overlay.height - kHeaderHeight - kBottomPadding - kOverlayPadding
    };
}

Rectangle draftCardBoundsFromGrid(Rectangle overlay, int index, int count)
{
    auto root = ui::grid(std::max(1, count), "worldActionCards");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = 14.0f;
    root->style(rootStyle);
    root->columnGap = 14.0f;
    root->fixedCellHeight = draftCardsArea(overlay).height;

    for (int i = 0; i < std::max(1, count); ++i) {
        auto card = std::make_unique<ui::PanelNode>("card" + std::to_string(i));
        card->style(ui::fixedHeight(draftCardsArea(overlay).height));
        root->add(std::move(card));
    }

    const Rectangle area = draftCardsArea(overlay);
    root->measure({area.width, area.height});
    root->layout(area);
    return nodeBounds(*root, ("card" + std::to_string(std::clamp(index, 0, std::max(1, count) - 1))).c_str());
}
} // namespace


Rectangle WorldActionOverlay::toggleBounds(int screenWidth)
{
    return {static_cast<float>(screenWidth) * 0.5f - 120.0f, 68.0f, 240.0f, 34.0f};
}

Rectangle WorldActionOverlay::overlayBounds(int screenWidth, int screenHeight)
{
    const float width = std::min(840.0f, static_cast<float>(screenWidth) - 96.0f);
    const float height = std::min(520.0f, static_cast<float>(screenHeight) - 120.0f);
    return {static_cast<float>(screenWidth) * 0.5f - width * 0.5f, static_cast<float>(screenHeight) * 0.5f - height * 0.5f, width, height};
}

Rectangle WorldActionOverlay::overlayBounds(int screenWidth, int screenHeight, const std::vector<WorldActionDraft>& drafts)
{
    const float width = std::min(840.0f, static_cast<float>(screenWidth) - 96.0f);
    Rectangle base{static_cast<float>(screenWidth) * 0.5f - width * 0.5f, 0.0f, width, 360.0f};
    const Rectangle cardsArea = draftCardsArea(base);
    const int count = std::max(1, static_cast<int>(drafts.size()));
    const float gap = 14.0f;
    const float cardWidth = (cardsArea.width - gap * static_cast<float>(count - 1)) / static_cast<float>(count);

    WorldActionCardView cardView;
    float cardHeight = 0.0f;
    for (const auto& draft : drafts) {
        cardHeight = std::max(cardHeight, cardView.preferredDraftHeight(cardWidth, draft));
    }
    if (cardHeight <= 0.0f) {
        cardHeight = cardsArea.height;
    }

    constexpr float kOverlayPadding = 20.0f;
    constexpr float kHeaderHeight = 66.0f;
    constexpr float kBottomPadding = 30.0f;
    const float wantedHeight = kHeaderHeight + kOverlayPadding + cardHeight + kBottomPadding;
    const float height = std::clamp(wantedHeight, 360.0f, std::max(360.0f, static_cast<float>(screenHeight) - 120.0f));
    return {static_cast<float>(screenWidth) * 0.5f - width * 0.5f, static_cast<float>(screenHeight) * 0.5f - height * 0.5f, width, height};
}

Rectangle WorldActionOverlay::draftCardBounds(Rectangle overlay, int index, int count)
{
    return draftCardBoundsFromGrid(overlay, index, count);
}

void WorldActionOverlay::draw(const UiContext& context) const
{
    if (context.state == nullptr || context.state->gameplayPhase != GameplayPhase::Planning || context.state->worldActionDraft.empty()) {
        return;
    }

    const Rectangle toggle = toggleBounds(context.screenWidth);
    if (context.state->eventPopupMode != EventPopupMode::None) {
        DrawRectangleRounded(toggle, 0.28f, 8, {17, 24, 34, 210});
        DrawRectangleRoundedLines(toggle, 0.28f, 8, {70, 86, 104, 120});
        IconRegistry::instance().drawIcon("action.world_toggle", {toggle.x + 12.0f, toggle.y + 8.0f, 18.0f, 18.0f}, {139, 148, 158, 190});
        drawTextClipped("Review Events First", {toggle.x + 38.0f, toggle.y + 9.0f, toggle.width - 50.0f, 16.0f}, 13, {139, 148, 158, 255});
        return;
    }
    const bool hasPick = context.state->selectedWorldActionIndex >= 0;
    DrawRectangleRounded(toggle, 0.28f, 8, context.state->worldActionDraftVisible ? Color{24, 34, 50, 245} : Color{17, 24, 34, 235});
    DrawRectangleRoundedLines(toggle, 0.28f, 8, hasPick ? Color{86, 210, 151, 210} : Color{245, 184, 76, 190});
    IconRegistry::instance().drawIcon("action.world_toggle", {toggle.x + 12.0f, toggle.y + 8.0f, 18.0f, 18.0f}, hasPick ? Color{86, 210, 151, 255} : Color{245, 184, 76, 255});
    const char* toggleText = context.state->worldActionDraftVisible ? "Hide World Actions" : (hasPick ? "Show World Actions" : "Pick World Action");
    drawTextClipped(toggleText, {toggle.x + 38.0f, toggle.y + 9.0f, toggle.width - 50.0f, 16.0f}, 13, {230, 237, 243, 255});

    if (!context.state->worldActionDraftVisible) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight, context.state->dockLayout);
    DrawRectangleRec(layout.worldView, {0, 0, 0, 128});

    const Rectangle overlay = overlayBounds(context.screenWidth, context.screenHeight, context.state->worldActionDraft);
    DrawRectangleRounded(overlay, 0.025f, 8, {9, 16, 27, 248});
    DrawRectangleRoundedLines(overlay, 0.025f, 8, {89, 196, 255, 120});
    DrawText("WORLD ACTION DRAFT", static_cast<int>(overlay.x + 22.0f), static_cast<int>(overlay.y + 20.0f), 14, {139, 148, 158, 255});
    drawTextClipped("Choose one strategic action for this planning phase. Node Actions unlock after a choice is made.", {overlay.x + 22.0f, overlay.y + 46.0f, overlay.width - 44.0f, 20.0f}, 13, {205, 213, 224, 255});

    const int count = static_cast<int>(context.state->worldActionDraft.size());
    const WorldActionCardView view;
    for (int i = 0; i < count; ++i) {
        view.drawDraft(
            draftCardBounds(overlay, i, count),
            context.state->worldActionDraft[static_cast<std::size_t>(i)],
            i == context.state->selectedWorldActionIndex,
            i == context.state->hoveredWorldActionIndex);
    }
}
