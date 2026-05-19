#include "ui/hud/ObjectivesDropdownPanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/hud/HudPanelPrimitives.hpp"
#include "ui/core/UiCore.hpp"
#include "ui/core/UiPrimitives.hpp"

#include <algorithm>
#include <memory>
#include <utility>


namespace {
Rectangle nodeBounds(const ui::UiNode& root, const char* id)
{
    if (const ui::UiNode* node = root.find(id); node != nullptr) {
        return node->bounds();
    }
    return {};
}

struct ObjectivesFieldLayout {
    Rectangle icon;
    Rectangle field;
};

ObjectivesFieldLayout computeObjectivesFieldLayout(Rectangle bounds)
{
    auto root = ui::horizontalStack("objectivesField");
    ui::LayoutStyle rootStyle;
    rootStyle.gap = 8.0f;
    root->style(rootStyle);

    auto icon = std::make_unique<ui::PanelNode>("icon");
    ui::LayoutStyle iconStyle = ui::fixedHeight(bounds.height);
    iconStyle.fixedWidth = 22.0f;
    iconStyle.widthMode = ui::SizeMode::Fixed;
    icon->style(iconStyle);
    root->add(std::move(icon));

    auto field = std::make_unique<ui::PanelNode>("field");
    ui::LayoutStyle fieldStyle;
    fieldStyle.widthMode = ui::SizeMode::Flex;
    fieldStyle.heightMode = ui::SizeMode::Flex;
    fieldStyle.flexGrow = 1.0f;
    field->style(fieldStyle);
    root->add(std::move(field));

    root->measure({bounds.width, bounds.height});
    root->layout(bounds);

    return {
        .icon = nodeBounds(*root, "icon"),
        .field = nodeBounds(*root, "field"),
    };
}
} // namespace


Rectangle ObjectivesDropdownPanel::menuBounds(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    const Rectangle field = hudObjectivesDroplistBounds(context.screenWidth);
    const auto& run = scenarioManager.run();

    constexpr float kTopPaddingAndProgress = 36.0f;
    constexpr float kBottomPadding = 16.0f;
    constexpr float kSectionSpacing = 20.0f;
    constexpr float kRowSpacing = 21.0f;

    float height = kTopPaddingAndProgress;
    height += kSectionSpacing + static_cast<float>(run.activeObjectiveIds.size()) * kRowSpacing;
    if (!run.completedObjectiveIds.empty()) {
        height += kSectionSpacing + static_cast<float>(run.completedObjectiveIds.size()) * kRowSpacing;
    }
    height += kSectionSpacing;
    height += static_cast<float>(std::max<std::size_t>(1, scenarioManager.definition().failureConditions.size())) * kRowSpacing;
    height += kSectionSpacing;
    height += static_cast<float>(std::max<std::size_t>(1, run.unlockedInterventions.size())) * kRowSpacing;
    if (!run.unlockedScenarioIds.empty()) {
        height += kSectionSpacing + static_cast<float>(run.unlockedScenarioIds.size()) * kRowSpacing;
    }
    if (scenarioManager.currentPhase() != nullptr) {
        height += kRowSpacing;
    }
    height += kBottomPadding;

    const float menuTop = field.y + field.height + 8.0f;
    return {field.x, menuTop, std::min(540.0f, field.width), height};
}

bool ObjectivesDropdownPanel::update(UiContext& context, const ScenarioManager& scenarioManager, Vector2 mouse)
{
    if (context.state == nullptr) {
        return false;
    }

    const Rectangle field = hudObjectivesDroplistBounds(context.screenWidth);
    if (CheckCollisionPointRec(mouse, field)) {
        context.state->objectivesDroplistOpen = !context.state->objectivesDroplistOpen;
        context.state->scenarioDroplistOpen = false;
        context.state->optionsMenuOpen = false;
        return true;
    }

    if (context.state->objectivesDroplistOpen && !hudPointInFieldOrMenu(mouse, field, menuBounds(context, scenarioManager))) {
        context.state->objectivesDroplistOpen = false;
    }
    return false;
}

void ObjectivesDropdownPanel::drawField(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr) {
        return;
    }

    const ObjectivesFieldLayout layout = computeObjectivesFieldLayout(hudObjectivesDroplistBounds(context.screenWidth));
    IconRegistry::instance().drawIcon("objective.dropdown", {layout.icon.x + 1.0f, layout.icon.y + 4.0f, 20.0f, 20.0f}, {245, 184, 76, 255});
    drawHudDroplistField(layout.field, "Objectives", scenarioManager.objectiveSummary(), context.state->objectivesDroplistOpen);
}

void ObjectivesDropdownPanel::drawMenu(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr || !context.state->objectivesDroplistOpen) {
        return;
    }

    const auto& run = scenarioManager.run();
    const Rectangle menu = menuBounds(context, scenarioManager);
    drawHudMenuShell(menu);
    const float progress = static_cast<float>(std::clamp(scenarioManager.run().objectiveProgress, 0.0, 1.0));
    DrawText("Progress", static_cast<int>(menu.x + 14.0f), static_cast<int>(menu.y + 12.0f), 12, {139, 148, 158, 255});
    DrawRectangleRounded({menu.x + 84.0f, menu.y + 14.0f, menu.width - 104.0f, 8.0f}, 0.5f, 8, {50, 58, 70, 220});
    DrawRectangleRounded({menu.x + 84.0f, menu.y + 14.0f, (menu.width - 104.0f) * progress, 8.0f}, 0.5f, 8, {86, 210, 151, 235});

    constexpr float kSectionSpacing = 20.0f;
    constexpr float kRowSpacing = 21.0f;

    float rowY = menu.y + 36.0f;
    DrawText("Active objectives", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {89, 196, 255, 255});
    rowY += kSectionSpacing;
    for (const auto& id : run.activeObjectiveIds) {
        const auto it = std::find_if(scenarioManager.definition().objectives.begin(), scenarioManager.definition().objectives.end(), [&id](const ScenarioObjective& objective) { return objective.id == id; });
        if (it != scenarioManager.definition().objectives.end()) {
            drawHudDetailRow("Active", it->summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += kRowSpacing;
        }
    }
    if (!run.completedObjectiveIds.empty()) {
        DrawText("Completed", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {86, 210, 151, 255});
        rowY += kSectionSpacing;
        for (const auto& id : run.completedObjectiveIds) {
            const auto it = std::find_if(scenarioManager.definition().objectives.begin(), scenarioManager.definition().objectives.end(), [&id](const ScenarioObjective& objective) { return objective.id == id; });
            if (it != scenarioManager.definition().objectives.end()) {
                drawHudDetailRow("Done", it->summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
                rowY += kRowSpacing;
            }
        }
    }

    DrawText("Failure limits", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {245, 184, 76, 255});
    rowY += kSectionSpacing;
    for (const auto& failure : scenarioManager.definition().failureConditions) {
        drawHudDetailRow("Limit", failure.summary, menu.x + 14.0f, rowY, menu.width - 28.0f);
        rowY += kRowSpacing;
    }
    if (scenarioManager.definition().failureConditions.empty()) {
        drawHudDetailRow("Limit", "No failure limits configured.", menu.x + 14.0f, rowY, menu.width - 28.0f);
        rowY += kRowSpacing;
    }
    DrawText("Unlocked actions", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {86, 210, 151, 255});
    rowY += kSectionSpacing;
    if (run.unlockedInterventions.empty()) {
        drawHudDetailRow("Action", "None yet.", menu.x + 14.0f, rowY, menu.width - 28.0f);
        rowY += kRowSpacing;
    }
    for (const auto mechanic : run.unlockedInterventions) {
        drawHudDetailRow("Action", std::string(MechanicRegistry::definition(mechanic).displayName), menu.x + 14.0f, rowY, menu.width - 28.0f);
        rowY += kRowSpacing;
    }
    if (!run.unlockedScenarioIds.empty()) {
        DrawText("Unlocked scenarios", static_cast<int>(menu.x + 14.0f), static_cast<int>(rowY + 1.0f), 12, {151, 111, 255, 255});
        rowY += kSectionSpacing;
        for (const auto& id : run.unlockedScenarioIds) {
            drawHudDetailRow("Scenario", id, menu.x + 14.0f, rowY, menu.width - 28.0f);
            rowY += kRowSpacing;
        }
    }
    if (const ScenarioPhase* phase = scenarioManager.currentPhase()) {
        drawHudDetailRow("Phase", phase->name, menu.x + 14.0f, rowY, menu.width - 28.0f);
    }
}
