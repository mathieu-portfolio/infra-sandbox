#include "ui/hud/PackDropdownPanel.hpp"

#include "ui/core/UiPrimitives.hpp"
#include "ui/hud/HudPanelPrimitives.hpp"

#include <algorithm>
#include <cstddef>

namespace {
Rectangle packRowBounds(Rectangle menu, int index)
{
    return {menu.x + 8.0f, menu.y + 100.0f + static_cast<float>(index) * 34.0f, menu.width - 16.0f, 30.0f};
}
}

Rectangle PackDropdownPanel::menuBounds(const UiContext& context, const content::ContentPackManager& packManager) const
{
    const Rectangle field = hudPackDroplistBounds(context.screenWidth);
    return {field.x, field.y + field.height + 8.0f, 390.0f, 112.0f + static_cast<float>(packManager.packs().size()) * 34.0f};
}

bool PackDropdownPanel::update(UiContext& context, const content::ContentPackManager& packManager, Vector2 mouse)
{
    if (context.state == nullptr) {
        return false;
    }

    const Rectangle field = hudPackDroplistBounds(context.screenWidth);
    if (CheckCollisionPointRec(mouse, field)) {
        context.state->packDroplistOpen = !context.state->packDroplistOpen;
        context.state->scenarioDroplistOpen = false;
        context.state->objectivesDroplistOpen = false;
        context.state->optionsMenuOpen = false;
        return true;
    }

    if (!context.state->packDroplistOpen) {
        return false;
    }

    const Rectangle menu = menuBounds(context, packManager);
    const auto& packs = packManager.packs();
    for (int i = 0; i < static_cast<int>(packs.size()); ++i) {
        if (!CheckCollisionPointRec(mouse, packRowBounds(menu, i))) {
            continue;
        }
        const auto& pack = packs[static_cast<std::size_t>(i)];
        if (pack.metadata.id != packManager.activePackId()) {
            context.state->requestedPackId = pack.metadata.id;
        }
        context.state->packDroplistOpen = false;
        return true;
    }

    if (!hudPointInFieldOrMenu(mouse, field, menu)) {
        context.state->packDroplistOpen = false;
    }
    return false;
}

void PackDropdownPanel::drawField(const UiContext& context, const content::ContentPackManager& packManager) const
{
    if (context.state == nullptr) {
        return;
    }
    const content::ContentPackInfo* active = packManager.activePack();
    const std::string label = active != nullptr ? active->metadata.displayName : "No Pack";
    drawHudDroplistField(hudPackDroplistBounds(context.screenWidth), "Pack", label, context.state->packDroplistOpen);
}

void PackDropdownPanel::drawMenu(const UiContext& context, const content::ContentPackManager& packManager) const
{
    if (context.state == nullptr || !context.state->packDroplistOpen) {
        return;
    }

    const auto& packs = packManager.packs();
    if (packs.empty()) {
        return;
    }

    const Rectangle menu = menuBounds(context, packManager);
    const Vector2 mouse = GetMousePosition();
    int previewIndex = 0;
    for (int i = 0; i < static_cast<int>(packs.size()); ++i) {
        if (packs[static_cast<std::size_t>(i)].metadata.id == packManager.activePackId()) {
            previewIndex = i;
        }
        if (CheckCollisionPointRec(mouse, packRowBounds(menu, i))) {
            previewIndex = i;
            break;
        }
    }
    previewIndex = std::clamp(previewIndex, 0, static_cast<int>(packs.size()) - 1);
    const auto& preview = packs[static_cast<std::size_t>(previewIndex)].metadata;

    drawHudMenuShell(menu);
    drawTextClipped(preview.description, {menu.x + 14.0f, menu.y + 12.0f, menu.width - 28.0f, 18.0f}, 14, {230, 237, 243, 255});
    drawHudDetailRow("Version", preview.version, menu.x + 14.0f, menu.y + 42.0f, menu.width - 28.0f);
    drawHudDetailRow("Author", preview.author.empty() ? "Unknown" : preview.author, menu.x + 14.0f, menu.y + 64.0f, menu.width - 28.0f);
    drawHudDetailRow("Default", preview.defaultScenarioId.empty() ? "First available scenario" : preview.defaultScenarioId, menu.x + 14.0f, menu.y + 86.0f, menu.width - 28.0f);

    for (int i = 0; i < static_cast<int>(packs.size()); ++i) {
        const Rectangle row = packRowBounds(menu, i);
        const auto& pack = packs[static_cast<std::size_t>(i)].metadata;
        const bool current = pack.id == packManager.activePackId();
        const bool previewed = i == previewIndex;
        DrawRectangleRounded(row, 0.12f, 6, current ? Color{37, 120, 255, 170} : previewed ? Color{38, 45, 56, 210} : Color{22, 27, 34, 150});
        DrawRectangleRoundedLines(row, 0.12f, 6, current || previewed ? Color{89, 196, 255, 190} : Color{70, 86, 104, 95});
        drawTextClipped(pack.displayName, {row.x + 10.0f, row.y + 7.0f, row.width - 126.0f, 16.0f}, 13, {230, 237, 243, 255});
        drawTextClipped(current ? "Active" : pack.version, {row.x + row.width - 108.0f, row.y + 7.0f, 98.0f, 16.0f}, 12, current ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
    }
}
