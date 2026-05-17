#include "ui/InterventionPanel.hpp"

#include "ui/ActionPanelModel.hpp"
#include "ui/ActionCardView.hpp"
#include "ui/IconRegistry.hpp"
#include "ui/UiLayout.hpp"
#include "ui/UiPrimitives.hpp"
#include "simulation/NodeDefinition.hpp"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace {
void section(Rectangle bounds, const char* title, int step)
{
    DrawRectangleRounded(bounds, 0.04f, 8, {22, 27, 34, 224});
    DrawRectangleRoundedLines(bounds, 0.04f, 8, {70, 86, 104, 95});
    char label[96];
    std::snprintf(label, sizeof(label), "%d.  %s", step, title);
    drawTextClipped(label, {bounds.x + 12.0f, bounds.y + 10.0f, bounds.width - 24.0f, 20.0f}, 16, {89, 196, 255, 255});
}

Color availableText(bool available)
{
    return available ? Color{230, 237, 243, 255} : Color{139, 148, 158, 170};
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
    case EngineeringDomain::Operations:
        return capacity.operations;
    case EngineeringDomain::Count:
        break;
    }
    return 0;
}

std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> domainUsage(const UiState& state)
{
    std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> usage{};
    for (const auto& planned : state.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            usage[static_cast<std::size_t>(cost.domain)] += cost.amount;
        }
    }
    return usage;
}

int totalUsage(const UiState& state)
{
    int total = 0;
    for (const auto& planned : state.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            total += cost.amount;
        }
    }
    return total;
}

std::string engineeringCostLabel(const std::vector<EngineeringCost>& costs)
{
    if (costs.empty()) {
        return "Engineering: none";
    }
    std::string label = "Engineering: ";
    for (std::size_t i = 0; i < costs.size(); ++i) {
        if (i > 0) {
            label += ", ";
        }
        label += engineeringDomainName(costs[i].domain);
        label += " ";
        label += std::to_string(costs[i].amount);
    }
    return label;
}

const char* shortDomainName(EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend: return "Front";
    case EngineeringDomain::Backend: return "Back";
    case EngineeringDomain::Infrastructure: return "Infra";
    case EngineeringDomain::Data: return "Data";
    case EngineeringDomain::Operations: return "Ops";
    case EngineeringDomain::Count: break;
    }
    return "";
}

const char* domainIcon(EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend: return "node.client";
    case EngineeringDomain::Backend: return "node.service";
    case EngineeringDomain::Infrastructure: return "action.scale_up";
    case EngineeringDomain::Data: return "node.database";
    case EngineeringDomain::Operations: return "action.queue";
    case EngineeringDomain::Count: break;
    }
    return "action.generic";
}

void drawCapacitySquares(Vector2 pos, int used, int max)
{
    constexpr float size = 10.0f;
    constexpr float gap = 4.0f;
    const int safeMax = std::max(0, max);
    const int safeUsed = std::clamp(used, 0, safeMax);

    for (int i = 0; i < safeMax; ++i) {
        const Rectangle square{
            pos.x + static_cast<float>(i) * (size + gap),
            pos.y,
            size,
            size,
        };

        const bool filled = i < safeUsed;
        DrawRectangleRounded(square, 0.25f, 4, filled ? Color{145, 109, 255, 255} : Color{44, 52, 64, 255});
        DrawRectangleRoundedLines(square, 0.25f, 4, filled ? Color{189, 135, 255, 255} : Color{70, 86, 104, 120});
    }
}

void drawCapacityRow(Rectangle row, const char* iconId, const char* label, int used, int max, Color labelColor)
{
    if (max <= 0) {
        return;
    }

    IconRegistry::instance().drawIcon(iconId, {row.x, row.y - 1.0f, 14.0f, 14.0f}, {139, 148, 158, 255});
    drawTextClipped(label, {row.x + 20.0f, row.y - 1.0f, 52.0f, 14.0f}, 11, labelColor);
    drawCapacitySquares({row.x + 78.0f, row.y}, used, max);
}

void drawEngineeringCapacityBlock(Rectangle bounds, const UiState& state)
{
    const auto usage = domainUsage(state);

    drawTextClipped("ENGINEERING CAPACITY", {bounds.x, bounds.y, bounds.width, 14.0f}, 11, {139, 148, 158, 255});

    float y = bounds.y + 24.0f;
    drawCapacityRow({bounds.x, y, bounds.width, 14.0f}, "action.generic", "Total", totalUsage(state), state.engineeringCapacity.total, {230, 237, 243, 255});
    y += 22.0f;

    for (int i = 0; i < static_cast<int>(EngineeringDomain::Count); ++i) {
        const auto domain = static_cast<EngineeringDomain>(i);
        const int cap = capacityForDomain(state.engineeringCapacity, domain);
        if (cap <= 0) {
            continue;
        }

        drawCapacityRow(
            {bounds.x, y, bounds.width, 14.0f},
            domainIcon(domain),
            shortDomainName(domain),
            usage[static_cast<std::size_t>(domain)],
            cap,
            {185, 195, 210, 255});
        y += 20.0f;
    }
}

std::string plannedLabel(const UiState& state)
{
    std::string label;
    int shown = 0;
    for (const auto& planned : state.plannedInterventions) {
        if (shown >= 2) {
            break;
        }
        if (!label.empty()) {
            label += ", ";
        }
        label += planned.actionName;
        ++shown;
    }
    if (state.plannedInterventions.size() > static_cast<std::size_t>(shown)) {
        label += ", +" + std::to_string(state.plannedInterventions.size() - static_cast<std::size_t>(shown));
    }
    return label;
}

const char* pressureSeverity(double value)
{
    if (value >= 0.72) return "High";
    if (value >= 0.42) return "Medium";
    if (value > 0.08) return "Low";
    return "Calm";
}

Color pressureColor(double value)
{
    if (value >= 0.72) return {235, 86, 100, 255};
    if (value >= 0.42) return {245, 184, 76, 255};
    if (value > 0.08) return {89, 196, 255, 255};
    return {86, 210, 151, 255};
}

double overallPressure(const NodePressure* pressure, const Node* node)
{
    if (pressure == nullptr) {
        return node != nullptr ? node->currentUtilization : 0.0;
    }
    return std::max({pressure->queuePressure, pressure->computePressure, pressure->latencyContribution, pressure->timeoutContribution, pressure->retryContribution, pressure->dependencyPressure, pressure->instability});
}

std::string categoryFilterLabel(const std::vector<ActionCard>& cards, int index)
{
    if (index == 0) return "All";
    std::vector<std::string> categories;
    for (const auto& card : cards) {
        if (!card.available || card.categories.empty()) {
            continue;
        }
        if (std::find(categories.begin(), categories.end(), card.categories.front()) == categories.end()) {
            categories.push_back(card.categories.front());
        }
    }
    if (index - 1 < static_cast<int>(categories.size())) {
        return categories[static_cast<std::size_t>(index - 1)];
    }
    static const char* fallback[] = {"Scaling", "Optimization", "Reliability"};
    return fallback[std::min(index - 1, 2)];
}

void drawFilterPill(Rectangle bounds, const std::string& label, bool active)
{
    DrawRectangleRounded(bounds, 0.22f, 8, active ? Color{97, 64, 196, 245} : Color{28, 36, 48, 210});
    DrawRectangleRoundedLines(bounds, 0.22f, 8, active ? Color{145, 109, 255, 210} : Color{70, 86, 104, 90});
    drawTextClipped(label, {bounds.x + 10.0f, bounds.y + 5.0f, bounds.width - 20.0f, 15.0f}, 12, active ? Color{244, 240, 255, 255} : Color{166, 176, 192, 255});
}


void drawWrappedTextClipped(const std::string& text, Rectangle bounds, int fontSize, Color color, float lineSpacing = 4.0f)
{
    if (text.empty() || bounds.width <= 0.0f || bounds.height <= 0.0f) {
        return;
    }

    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height));

    float y = bounds.y;
    std::string line;
    std::string word;

    auto flushWord = [&]() {
        if (word.empty()) {
            return;
        }
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && MeasureText(candidate.c_str(), fontSize) > bounds.width) {
            if (y + static_cast<float>(fontSize) > bounds.y + bounds.height) {
                word.clear();
                return;
            }
            DrawText(line.c_str(), static_cast<int>(bounds.x), static_cast<int>(y), fontSize, color);
            y += static_cast<float>(fontSize) + lineSpacing;
            line = word;
        } else {
            line = candidate;
        }
        word.clear();
    };

    for (char c : text) {
        if (c == ' ' || c == '\n' || c == '\t') {
            flushWord();
            if (c == '\n') {
                if (!line.empty() && y + static_cast<float>(fontSize) <= bounds.y + bounds.height) {
                    DrawText(line.c_str(), static_cast<int>(bounds.x), static_cast<int>(y), fontSize, color);
                }
                y += static_cast<float>(fontSize) + lineSpacing;
                line.clear();
            }
        } else {
            word.push_back(c);
        }
    }
    flushWord();

    if (!line.empty() && y + static_cast<float>(fontSize) <= bounds.y + bounds.height) {
        DrawText(line.c_str(), static_cast<int>(bounds.x), static_cast<int>(y), fontSize, color);
    }

    EndScissorMode();
}

void drawMetricBar(Rectangle bounds, const char* label, double value, Color color)
{
    drawTextClipped(label, {bounds.x, bounds.y, 92.0f, 16.0f}, 13, {185, 195, 210, 255});
    DrawRectangleRounded({bounds.x + 102.0f, bounds.y + 5.0f, bounds.width - 154.0f, 5.0f}, 0.5f, 6, {45, 55, 68, 255});
    DrawRectangleRounded({bounds.x + 102.0f, bounds.y + 5.0f, (bounds.width - 154.0f) * static_cast<float>(std::clamp(value, 0.0, 1.0)), 5.0f}, 0.5f, 6, color);
    char text[24];
    std::snprintf(text, sizeof(text), "%.0f%%", value * 100.0);
    DrawText(text, static_cast<int>(bounds.x + bounds.width - 42.0f), static_cast<int>(bounds.y - 1.0f), 13, {230, 237, 243, 255});
}

struct RightPanelLayout {
    Rectangle header{};
    Rectangle tabs{};
    Rectangle overview{};
    Rectangle actions{};
    Rectangle actionList{};
    Rectangle message{};
    Rectangle locked{};
    Rectangle button{};
};

RightPanelLayout computeRightPanelLayout(Rectangle sidebar)
{
    constexpr float pad = 18.0f;
    constexpr float gap = 14.0f;
    constexpr float headerHeight = 64.0f;
    constexpr float tabsHeight = 46.0f;
    constexpr float overviewHeight = 208.0f;
    constexpr float actionsHeaderHeight = 206.0f;
    constexpr float messageHeight = 26.0f;
    constexpr float lockedHeight = 72.0f;
    constexpr float buttonHeight = 46.0f;

    RightPanelLayout layout;
    const float contentX = sidebar.x + pad;
    const float contentWidth = sidebar.width - pad * 2.0f;
    float y = sidebar.y + pad;
    layout.header = {contentX, y, contentWidth, headerHeight};
    y += headerHeight + gap;
    layout.tabs = {contentX, y, contentWidth, tabsHeight};
    y += tabsHeight + gap;
    layout.overview = {contentX, y, contentWidth, overviewHeight};
    y += overviewHeight + gap;

    layout.button = {contentX, sidebar.y + sidebar.height - pad - buttonHeight, contentWidth, buttonHeight};
    layout.locked = {contentX, layout.button.y - gap - lockedHeight, contentWidth, lockedHeight};
    layout.message = {contentX, layout.locked.y - gap - messageHeight, contentWidth, messageHeight};
    layout.actions = {contentX, y, contentWidth, std::max(0.0f, layout.message.y - gap - y)};
    layout.actionList = {contentX, y + actionsHeaderHeight, contentWidth, std::max(0.0f, layout.actions.height - actionsHeaderHeight)};
    return layout;
}
}

void InterventionPanel::update(UiContext& context, const Simulation& simulation)
{
    if (context.state == nullptr) {
        return;
    }
    context.state->hoveredActionIndex = -1;
    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);
    const Vector2 mouse = GetMousePosition();
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        if (CheckCollisionPointRec(mouse, cards[static_cast<std::size_t>(i)].bounds)) {
            context.state->hoveredActionIndex = i;
            return;
        }
    }
}

void InterventionPanel::draw(const UiContext& context, const Simulation& simulation) const
{
    if (context.state == nullptr) {
        return;
    }

    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const Rectangle sidebar = layout.rightSidebar;
    const RightPanelLayout panel = computeRightPanelLayout(sidebar);
    DrawRectangleRounded(sidebar, 0.018f, 8, {9, 16, 27, 242});
    DrawRectangleRoundedLines(sidebar, 0.018f, 8, {62, 82, 112, 130});

    const Node* selected = simulation.graph().node(context.state->selection.nodeId);
    const NodePressure* selectedPressure = selected != nullptr ? simulation.pressureAnalysis().pressureForNode(selected->id) : nullptr;
    const Rectangle header = panel.header;
    if (selected != nullptr) {
        DrawRectangleRounded({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {37, 50, 82, 255});
        DrawRectangleRoundedLines({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {130, 93, 255, 220});
        IconRegistry::instance().drawIcon("node.service", {header.x + 14.0f, header.y + 18.0f, 26.0f, 26.0f}, {89, 196, 255, 255});
        drawTextClipped(selected->name, {header.x + 70.0f, header.y + 4.0f, header.width - 150.0f, 30.0f}, 25, {241, 245, 249, 255});
        const char* region = selected->hasGeoLocation ? selected->geoLocation.regionName.c_str() : "local";
        drawTextClipped(region, {header.x + 70.0f, header.y + 40.0f, 140.0f, 20.0f}, 14, {166, 176, 192, 255});
        const double load = overallPressure(selectedPressure, selected);
        const char* status = load >= 0.72 ? "Under heavy load" : load >= 0.42 ? "Moderate pressure" : "Stable";
        DrawCircle(static_cast<int>(header.x + header.width - 126.0f), static_cast<int>(header.y + 17.0f), 6.0f, pressureColor(load));
        drawTextClipped(status, {header.x + header.width - 112.0f, header.y + 6.0f, 108.0f, 18.0f}, 14, pressureColor(load));
    } else {
        DrawRectangleRounded({header.x, header.y + 4.0f, 52.0f, 52.0f}, 0.12f, 8, {37, 50, 82, 255});
        IconRegistry::instance().drawIcon("node.service", {header.x + 14.0f, header.y + 18.0f, 26.0f, 26.0f}, {139, 148, 158, 255});
        drawTextClipped("Select a Node", {header.x + 70.0f, header.y + 6.0f, header.width - 84.0f, 30.0f}, 25, {241, 245, 249, 255});
        drawTextClipped("Inspect local actions and pressures", {header.x + 70.0f, header.y + 42.0f, header.width - 84.0f, 20.0f}, 14, {166, 176, 192, 255});
    }

    DrawLine(static_cast<int>(sidebar.x), static_cast<int>(panel.tabs.y - 5.0f), static_cast<int>(sidebar.x + sidebar.width), static_cast<int>(panel.tabs.y - 5.0f), {31, 42, 58, 255});
    const char* tabs[] = {"Overview", "Metrics", "Traffic", "Dependencies"};
    for (int i = 0; i < 4; ++i) {
        const float tabWidth = panel.tabs.width / 4.0f;
        const float x = panel.tabs.x + static_cast<float>(i) * tabWidth;
        drawTextClipped(tabs[i], {x + 4.0f, panel.tabs.y + 10.0f, tabWidth - 8.0f, 18.0f}, 13, i == 0 ? Color{230, 237, 243, 255} : Color{139, 148, 158, 255});
        if (i == 0) {
            DrawRectangleRounded({x, panel.tabs.y + panel.tabs.height - 4.0f, tabWidth - 10.0f, 3.0f}, 0.4f, 6, {145, 109, 255, 255});
        }
    }

    const Rectangle overview = panel.overview;
    const Rectangle description{overview.x, overview.y, overview.width * 0.58f - 7.0f, overview.height};
    const Rectangle status{description.x + description.width + 14.0f, overview.y, overview.width - description.width - 14.0f, overview.height};
    DrawRectangleRounded(description, 0.035f, 8, {13, 34, 47, 226});
    DrawRectangleRoundedLines(description, 0.035f, 8, {48, 95, 112, 115});
    DrawText("OVERVIEW", static_cast<int>(description.x + 14.0f), static_cast<int>(description.y + 15.0f), 12, {166, 176, 192, 255});
    if (selected != nullptr) {
        const auto& nodeDef = NodeRegistry::definition(selected->type);
        drawWrappedTextClipped(std::string("This ") + std::string(nodeDef.displayName) + " handles local request flow and participates in the dependency path.", {description.x + 16.0f, description.y + 44.0f, description.width - 32.0f, 68.0f}, 13, {205, 213, 224, 255});
        drawWrappedTextClipped(selectedPressure != nullptr && !selectedPressure->explanation.empty() ? selectedPressure->explanation : "Watch traffic, queue depth, and dependency pressure before committing changes.", {description.x + 16.0f, description.y + 122.0f, description.width - 32.0f, description.height - 134.0f}, 12, {166, 176, 192, 255});
    } else {
        drawWrappedTextClipped("Select an infrastructure node to see its role, health, pressures, and contextual actions.", {description.x + 16.0f, description.y + 44.0f, description.width - 32.0f, description.height - 58.0f}, 13, {205, 213, 224, 255});
    }

    DrawRectangleRounded(status, 0.035f, 8, {18, 24, 34, 230});
    DrawRectangleRoundedLines(status, 0.035f, 8, {70, 86, 104, 110});
    const double overall = overallPressure(selectedPressure, selected);
    drawTextClipped("HEALTH", {status.x + 14.0f, status.y + 15.0f, 80.0f, 16.0f}, 14, {205, 213, 224, 255});
    drawTextClipped(pressureSeverity(overall), {status.x + status.width - 78.0f, status.y + 15.0f, 64.0f, 16.0f}, 13, pressureColor(overall));
    drawMetricBar({status.x + 14.0f, status.y + 48.0f, status.width - 28.0f, 18.0f}, "Utilization", selected != nullptr ? selected->currentUtilization : 0.0, pressureColor(selected != nullptr ? selected->currentUtilization : 0.0));
    drawMetricBar({status.x + 14.0f, status.y + 78.0f, status.width - 28.0f, 18.0f}, "Queue", selectedPressure != nullptr ? selectedPressure->queuePressure : 0.0, pressureColor(selectedPressure != nullptr ? selectedPressure->queuePressure : 0.0));
    drawMetricBar({status.x + 14.0f, status.y + 108.0f, status.width - 28.0f, 18.0f}, "Latency", selectedPressure != nullptr ? selectedPressure->latencyContribution : 0.0, pressureColor(selectedPressure != nullptr ? selectedPressure->latencyContribution : 0.0));
    DrawText("PRIMARY PRESSURES", static_cast<int>(status.x + 14.0f), static_cast<int>(status.y + 138.0f), 11, {166, 176, 192, 255});
    const PressureCategory pressures[] = {selectedPressure != nullptr ? selectedPressure->dominant : PressureCategory::None, PressureCategory::QueuePressure, PressureCategory::PersistencePressure};
    const double pressureValues[] = {overall, selectedPressure != nullptr ? selectedPressure->queuePressure : 0.0, selectedPressure != nullptr ? selectedPressure->dependencyPressure : 0.0};
    for (int i = 0; i < 3; ++i) {
        const float y = status.y + 156.0f + static_cast<float>(i) * 16.0f;
        DrawCircle(static_cast<int>(status.x + 18.0f), static_cast<int>(y + 6.0f), 4.0f, pressureColor(pressureValues[i]));
        drawTextClipped(pressureCategoryName(pressures[i]), {status.x + 28.0f, y, status.width - 100.0f, 12.0f}, 11, {230, 237, 243, 255});
        drawTextClipped(pressureSeverity(pressureValues[i]), {status.x + status.width - 66.0f, y, 52.0f, 12.0f}, 11, pressureColor(pressureValues[i]));
    }

    const ActionPanelModel model;
    const auto cards = model.buildCards(simulation, *context.state, context.screenWidth, context.screenHeight);
    const Rectangle actions = panel.actions;
    DrawText("AVAILABLE ACTIONS", static_cast<int>(actions.x), static_cast<int>(actions.y), 14, {230, 237, 243, 255});
    drawEngineeringCapacityBlock({actions.x, actions.y + 24.0f, actions.width, 142.0f}, *context.state);
    for (int i = 0; i < 4; ++i) {
        const float w = i == 0 ? 48.0f : 96.0f;
        drawFilterPill({actions.x + static_cast<float>(i) * 108.0f, actions.y + 176.0f, w, 26.0f}, categoryFilterLabel(cards, i), i == 0);
    }
    BeginScissorMode(static_cast<int>(panel.actionList.x - 2.0f), static_cast<int>(panel.actionList.y), static_cast<int>(panel.actionList.width + 4.0f), static_cast<int>(panel.actionList.height));
    if (cards.empty()) {
        drawTextClipped("Select an API, database, cache, or queue node to see contextual actions.", {panel.actionList.x, panel.actionList.y + 4.0f, panel.actionList.width, 18.0f}, 13, {139, 148, 158, 255});
    }
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        const auto& card = cards[static_cast<std::size_t>(i)];
        if (!card.available || card.bounds.height <= 0.0f) {
            continue;
        }
        const bool highlighted = i == context.state->hoveredActionIndex || i == context.state->selectedActionIndex;
        ActionCardView{}.draw(card, highlighted);
    }
    EndScissorMode();

    const Rectangle locked = panel.locked;
    DrawRectangleRounded(locked, 0.045f, 8, {17, 24, 34, 230});
    DrawRectangleRoundedLines(locked, 0.045f, 8, {70, 86, 104, 110});
    IconRegistry::instance().drawIcon("action.generic", {locked.x + 16.0f, locked.y + 24.0f, 24.0f, 24.0f}, {139, 148, 158, 180});
    int lockedCount = 0;
    std::string lockedReason = "More actions will be discovered as scenarios unlock concepts.";
    for (const auto& card : cards) {
        if (!card.available) {
            ++lockedCount;
            if (!card.unavailableReason.empty()) {
                lockedReason = card.unavailableReason;
            }
        }
    }
    drawTextClipped(lockedCount > 0 ? std::to_string(lockedCount) + " locked action(s)" : "Locked actions", {locked.x + 56.0f, locked.y + 17.0f, locked.width - 72.0f, 20.0f}, 13, {166, 176, 192, 255});
    drawTextClipped(lockedReason, {locked.x + 56.0f, locked.y + 42.0f, locked.width - 72.0f, 18.0f}, 12, {139, 148, 158, 255});

    if (context.state->placementActive) {
        const PlacementCandidateGenerator generator;
        const auto candidates = generator.generate(simulation, context.state->activeMutation);
        if (!candidates.empty()) {
            const int index = std::clamp(context.state->placementCandidateIndex, 0, static_cast<int>(candidates.size()) - 1);
            drawTextClipped("Placement: " + candidates[static_cast<std::size_t>(index)].displayName, panel.message, 12, {89, 196, 255, 255});
        }
    }

    DrawRectangleRounded(panel.button, 0.08f, 8, {97, 64, 196, static_cast<unsigned char>(context.state->placementActive ? 255 : 150)});
    const bool hasSelectedAction = context.state->selectedActionIndex >= 0;
    DrawText(context.state->placementActive ? "Queue Placement" : (hasSelectedAction ? "Queue Action" : "Select an Action"), static_cast<int>(panel.button.x + panel.button.width * 0.5f - 58.0f), static_cast<int>(panel.button.y + 14.0f), 15, {230, 237, 243, 255});

    if (!context.state->latestFeedback.empty() && !context.state->placementActive) {
        drawTextClipped(context.state->latestFeedback, panel.message, 13, {245, 184, 76, 255});
    }
    if (!context.state->plannedInterventions.empty()) {
        drawTextClipped("Planned: " + plannedLabel(*context.state), panel.message, 12, {86, 210, 151, 255});
    } else if (!context.state->resolutionSummaries.empty()) {
        drawTextClipped(context.state->resolutionSummaries.front(), panel.message, 12, {86, 210, 151, 255});
    }
}
