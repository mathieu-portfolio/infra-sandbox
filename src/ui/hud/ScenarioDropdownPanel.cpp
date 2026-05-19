#include "ui/hud/ScenarioDropdownPanel.hpp"

#include "ui/hud/HudPanelPrimitives.hpp"
#include "ui/core/UiPrimitives.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

namespace {
int scenarioIndexForId(const std::string& id)
{
    const auto scenarios = ScenarioRegistry::createAll();
    for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
        if (scenarios[static_cast<std::size_t>(i)].id == id) {
            return i;
        }
    }
    return -1;
}

Rectangle scenarioRowBounds(Rectangle menu, int index)
{
    return {menu.x + 8.0f, menu.y + 120.0f + static_cast<float>(index) * 34.0f, menu.width - 16.0f, 30.0f};
}


template <typename T, typename = void>
struct HasName : std::false_type {};
template <typename T>
struct HasName<T, std::void_t<decltype(std::declval<T>().name)>> : std::true_type {};

template <typename T, typename = void>
struct HasLabel : std::false_type {};
template <typename T>
struct HasLabel<T, std::void_t<decltype(std::declval<T>().label)>> : std::true_type {};

template <typename T, typename = void>
struct HasId : std::false_type {};
template <typename T>
struct HasId<T, std::void_t<decltype(std::declval<T>().id)>> : std::true_type {};

template <typename T, typename = void>
struct HasArchetype : std::false_type {};
template <typename T>
struct HasArchetype<T, std::void_t<decltype(std::declval<T>().archetype)>> : std::true_type {};

template <typename T, typename = void>
struct HasArchetypeName : std::false_type {};
template <typename T>
struct HasArchetypeName<T, std::void_t<decltype(std::declval<T>().archetypeName)>> : std::true_type {};

template <typename T, typename = void>
struct HasContent : std::false_type {};
template <typename T>
struct HasContent<T, std::void_t<decltype(std::declval<T>().content)>> : std::true_type {};

template <typename T, typename = void>
struct HasScenarioContent : std::false_type {};
template <typename T>
struct HasScenarioContent<T, std::void_t<decltype(std::declval<T>().scenario)>> : std::true_type {};

template <typename T, typename = void>
struct HasFocus : std::false_type {};
template <typename T>
struct HasFocus<T, std::void_t<decltype(std::declval<T>().focus)>> : std::true_type {};

template <typename T, typename = void>
struct HasFocusName : std::false_type {};
template <typename T>
struct HasFocusName<T, std::void_t<decltype(std::declval<T>().focusName)>> : std::true_type {};

template <typename T, typename = void>
struct HasScenarioFocus : std::false_type {};
template <typename T>
struct HasScenarioFocus<T, std::void_t<decltype(std::declval<T>().scenarioFocus)>> : std::true_type {};

template <typename T, typename = void>
struct HasFocusAreas : std::false_type {};
template <typename T>
struct HasFocusAreas<T, std::void_t<decltype(std::declval<T>().focusAreas)>> : std::true_type {};

template <typename T, typename = void>
struct HasFocusTags : std::false_type {};
template <typename T>
struct HasFocusTags<T, std::void_t<decltype(std::declval<T>().focusTags)>> : std::true_type {};

template <typename T, typename = void>
struct HasModifiers : std::false_type {};
template <typename T>
struct HasModifiers<T, std::void_t<decltype(std::declval<T>().modifiers)>> : std::true_type {};

template <typename T, typename = void>
struct HasScenarioModifiers : std::false_type {};
template <typename T>
struct HasScenarioModifiers<T, std::void_t<decltype(std::declval<T>().scenarioModifiers)>> : std::true_type {};

template <typename T, typename = void>
struct HasModifierIds : std::false_type {};
template <typename T>
struct HasModifierIds<T, std::void_t<decltype(std::declval<T>().modifierIds)>> : std::true_type {};

template <typename T, typename = void>
struct HasActiveModifiers : std::false_type {};
template <typename T>
struct HasActiveModifiers<T, std::void_t<decltype(std::declval<T>().activeModifiers)>> : std::true_type {};

template <typename T, typename = void>
struct HasEnabledModifiers : std::false_type {};
template <typename T>
struct HasEnabledModifiers<T, std::void_t<decltype(std::declval<T>().enabledModifiers)>> : std::true_type {};

template <typename T, typename = void>
struct HasEmptyAndSize : std::false_type {};
template <typename T>
struct HasEmptyAndSize<T, std::void_t<decltype(std::declval<T>().empty()), decltype(std::declval<T>().size())>> : std::true_type {};

template <typename T>
std::string scalarPreviewValue(const T& value)
{
    if constexpr (std::is_convertible_v<T, std::string>) {
        return std::string(value);
    } else if constexpr (std::is_convertible_v<T, const char*>) {
        return std::string(value);
    } else if constexpr (HasName<T>::value) {
        return scalarPreviewValue(value.name);
    } else if constexpr (HasLabel<T>::value) {
        return scalarPreviewValue(value.label);
    } else if constexpr (HasId<T>::value) {
        return scalarPreviewValue(value.id);
    } else {
        return "Configured";
    }
}

template <typename T>
std::string listPreviewValue(const T& values)
{
    if constexpr (!HasEmptyAndSize<T>::value) {
        return scalarPreviewValue(values);
    } else {
        if (values.empty()) {
            return "None";
        }
        std::string result;
        int shown = 0;
        for (const auto& value : values) {
            if (shown >= 3) {
                break;
            }
            if (!result.empty()) {
                result += ", ";
            }
            result += scalarPreviewValue(value);
            ++shown;
        }
        if (values.size() > static_cast<std::size_t>(shown)) {
            result += ", +" + std::to_string(values.size() - static_cast<std::size_t>(shown));
        }
        return result;
    }
}

template <typename T>
std::string previewArchetype(const T& scenario, const std::string& fallback)
{
    if constexpr (HasArchetype<T>::value) {
        return scalarPreviewValue(scenario.archetype);
    } else if constexpr (HasArchetypeName<T>::value) {
        return scalarPreviewValue(scenario.archetypeName);
    } else if constexpr (HasContent<T>::value) {
        return previewArchetype(scenario.content, fallback);
    } else if constexpr (HasScenarioContent<T>::value) {
        return previewArchetype(scenario.scenario, fallback);
    } else {
        return fallback;
    }
}

template <typename T>
std::string previewFocus(const T& scenario, const std::string& fallback)
{
    if constexpr (HasFocus<T>::value) {
        return scalarPreviewValue(scenario.focus);
    } else if constexpr (HasFocusName<T>::value) {
        return scalarPreviewValue(scenario.focusName);
    } else if constexpr (HasScenarioFocus<T>::value) {
        return scalarPreviewValue(scenario.scenarioFocus);
    } else if constexpr (HasFocusAreas<T>::value) {
        return listPreviewValue(scenario.focusAreas);
    } else if constexpr (HasFocusTags<T>::value) {
        return listPreviewValue(scenario.focusTags);
    } else if constexpr (HasContent<T>::value) {
        return previewFocus(scenario.content, fallback);
    } else if constexpr (HasScenarioContent<T>::value) {
        return previewFocus(scenario.scenario, fallback);
    } else {
        return fallback;
    }
}

template <typename T>
std::string previewModifiers(const T& scenario, const std::string& fallback)
{
    if constexpr (HasModifiers<T>::value) {
        return listPreviewValue(scenario.modifiers);
    } else if constexpr (HasScenarioModifiers<T>::value) {
        return listPreviewValue(scenario.scenarioModifiers);
    } else if constexpr (HasModifierIds<T>::value) {
        return listPreviewValue(scenario.modifierIds);
    } else if constexpr (HasActiveModifiers<T>::value) {
        return listPreviewValue(scenario.activeModifiers);
    } else if constexpr (HasEnabledModifiers<T>::value) {
        return listPreviewValue(scenario.enabledModifiers);
    } else if constexpr (HasContent<T>::value) {
        return previewModifiers(scenario.content, fallback);
    } else if constexpr (HasScenarioContent<T>::value) {
        return previewModifiers(scenario.scenario, fallback);
    } else {
        return fallback;
    }
}
}

Rectangle ScenarioDropdownPanel::menuBounds(const UiContext& context) const
{
    const Rectangle field = hudScenarioDroplistBounds(context.screenWidth);
    const auto scenarios = ScenarioRegistry::createAll();
    return {field.x, field.y + field.height + 8.0f, 390.0f, 132.0f + static_cast<float>(scenarios.size()) * 34.0f};
}

bool ScenarioDropdownPanel::update(UiContext& context, const ScenarioManager& scenarioManager, Vector2 mouse)
{
    if (context.state == nullptr) {
        return false;
    }

    const Rectangle field = hudScenarioDroplistBounds(context.screenWidth);
    if (CheckCollisionPointRec(mouse, field)) {
        context.state->scenarioDroplistOpen = !context.state->scenarioDroplistOpen;
        context.state->objectivesDroplistOpen = false;
        context.state->optionsMenuOpen = false;
        return true;
    }

    if (!context.state->scenarioDroplistOpen) {
        return false;
    }

    const Rectangle menu = menuBounds(context);
    const auto scenarios = ScenarioRegistry::createAll();
    const int currentIndex = scenarioIndexForId(scenarioManager.staticDefinition().id);
    for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
        if (!CheckCollisionPointRec(mouse, scenarioRowBounds(menu, i))) {
            continue;
        }
        if (i != currentIndex && scenarioManager.isScenarioUnlocked(scenarios[static_cast<std::size_t>(i)])) {
            context.state->requestedScenarioId = scenarios[static_cast<std::size_t>(i)].id;
        }
        context.state->scenarioDroplistOpen = false;
        return true;
    }

    if (!hudPointInFieldOrMenu(mouse, field, menu)) {
        context.state->scenarioDroplistOpen = false;
    }
    return false;
}

void ScenarioDropdownPanel::drawField(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr) {
        return;
    }
    drawHudDroplistField(hudScenarioDroplistBounds(context.screenWidth), "Scenario", scenarioManager.definition().name, context.state->scenarioDroplistOpen);
}

void ScenarioDropdownPanel::drawMenu(const UiContext& context, const ScenarioManager& scenarioManager) const
{
    if (context.state == nullptr || !context.state->scenarioDroplistOpen) {
        return;
    }

    const auto scenarios = ScenarioRegistry::createAll();
    const int currentIndex = scenarioIndexForId(scenarioManager.staticDefinition().id);
    const Rectangle menu = menuBounds(context);
    const Vector2 mouse = GetMousePosition();

    int previewIndex = currentIndex >= 0 ? currentIndex : 0;
    for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
        if (CheckCollisionPointRec(mouse, scenarioRowBounds(menu, i))) {
            previewIndex = i;
            break;
        }
    }
    previewIndex = std::max(0, std::min(previewIndex, static_cast<int>(scenarios.size()) - 1));
    const auto& preview = scenarios[static_cast<std::size_t>(previewIndex)];
    const bool previewIsCurrent = previewIndex == currentIndex;

    drawHudMenuShell(menu);
    drawTextClipped(preview.description, {menu.x + 14.0f, menu.y + 12.0f, menu.width - 28.0f, 18.0f}, 14, {230, 237, 243, 255});
    drawHudDetailRow("Archetype", previewIsCurrent ? scenarioManager.archetypeSummary() : previewArchetype(preview, "Not specified"), menu.x + 14.0f, menu.y + 42.0f, menu.width - 28.0f);
    drawHudDetailRow("Tier", progressionTierName(preview.minimumTier), menu.x + 14.0f, menu.y + 64.0f, menu.width - 28.0f);
    drawHudDetailRow("Focus", previewIsCurrent ? scenarioManager.focusSummary() : previewFocus(preview, "Not specified"), menu.x + 14.0f, menu.y + 86.0f, menu.width - 28.0f);
    drawHudDetailRow("Modifiers", previewIsCurrent ? scenarioManager.activeModifiersSummary() : previewModifiers(preview, "None"), menu.x + 14.0f, menu.y + 108.0f, menu.width - 28.0f);
    for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
        const Rectangle row = scenarioRowBounds(menu, i);
        const bool current = i == currentIndex;
        const bool previewed = i == previewIndex;
        const bool unlocked = scenarioManager.isScenarioUnlocked(scenarios[static_cast<std::size_t>(i)]);
        DrawRectangleRounded(row, 0.12f, 6, current ? Color{37, 120, 255, 170} : previewed ? Color{38, 45, 56, 210} : Color{22, 27, 34, 150});
        DrawRectangleRoundedLines(row, 0.12f, 6, current || previewed ? Color{89, 196, 255, 190} : Color{70, 86, 104, 95});
        drawTextClipped(scenarios[static_cast<std::size_t>(i)].name, {row.x + 10.0f, row.y + 7.0f, row.width - 126.0f, 16.0f}, 13, unlocked ? Color{230, 237, 243, 255} : Color{90, 107, 126, 255});
        drawTextClipped(unlocked ? progressionTierName(scenarios[static_cast<std::size_t>(i)].minimumTier) : "Locked", {row.x + row.width - 108.0f, row.y + 7.0f, 98.0f, 16.0f}, 12, current ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
    }
}
