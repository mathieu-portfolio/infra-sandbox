#include "ui/actions/WorldActionOverlay.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"
#include "ui/UiPrimitives.hpp"
#include "ui/actions/cards/WorldActionCardView.hpp"

#include <algorithm>

Rectangle WorldActionOverlay::toggleBounds(int screenWidth)
{
    return {static_cast<float>(screenWidth) * 0.5f - 120.0f, 68.0f, 240.0f, 34.0f};
}

Rectangle WorldActionOverlay::overlayBounds(int screenWidth, int screenHeight)
{
    const float width = std::min(840.0f, static_cast<float>(screenWidth) - 96.0f);
    const float height = std::min(360.0f, static_cast<float>(screenHeight) - 160.0f);
    return {static_cast<float>(screenWidth) * 0.5f - width * 0.5f, static_cast<float>(screenHeight) * 0.5f - height * 0.5f, width, height};
}

Rectangle WorldActionOverlay::draftCardBounds(Rectangle overlay, int index, int count)
{
    constexpr float gap = 14.0f;
    const float contentX = overlay.x + 20.0f;
    const float contentWidth = overlay.width - 40.0f;
    const float width = (contentWidth - gap * static_cast<float>(std::max(0, count - 1))) / static_cast<float>(std::max(1, count));
    return {contentX + static_cast<float>(index) * (width + gap), overlay.y + 86.0f, width, overlay.height - 116.0f};
}

void WorldActionOverlay::draw(const UiContext& context) const
{
    if (context.state == nullptr || context.state->gameplayPhase != GameplayPhase::Planning || context.state->worldActionDraft.empty()) {
        return;
    }

    const Rectangle toggle = toggleBounds(context.screenWidth);
    const bool hasPick = context.state->selectedWorldActionIndex >= 0;
    DrawRectangleRounded(toggle, 0.28f, 8, context.state->worldActionDraftVisible ? Color{24, 34, 50, 245} : Color{17, 24, 34, 235});
    DrawRectangleRoundedLines(toggle, 0.28f, 8, hasPick ? Color{86, 210, 151, 210} : Color{245, 184, 76, 190});
    IconRegistry::instance().drawIcon("action.generic", {toggle.x + 12.0f, toggle.y + 8.0f, 18.0f, 18.0f}, hasPick ? Color{86, 210, 151, 255} : Color{245, 184, 76, 255});
    const char* toggleText = context.state->worldActionDraftVisible ? "Hide World Actions" : (hasPick ? "Show World Actions" : "Pick World Action");
    drawTextClipped(toggleText, {toggle.x + 38.0f, toggle.y + 9.0f, toggle.width - 50.0f, 16.0f}, 13, {230, 237, 243, 255});

    if (!context.state->worldActionDraftVisible) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    DrawRectangleRec(layout.worldView, {0, 0, 0, 128});

    const Rectangle overlay = overlayBounds(context.screenWidth, context.screenHeight);
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
