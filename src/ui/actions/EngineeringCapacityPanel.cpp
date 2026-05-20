#include "ui/actions/EngineeringCapacityPanel.hpp"

#include "ui/widgets/IconRegistry.hpp"
#include "ui/core/UiPrimitives.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace {
int capacityForDomain(const EngineeringCapacity& capacity, EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend: return capacity.frontend;
    case EngineeringDomain::Backend: return capacity.backend;
    case EngineeringDomain::Infrastructure: return capacity.infrastructure;
    case EngineeringDomain::Data: return capacity.data;
    case EngineeringDomain::Count: break;
    }
    return 0;
}

std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> domainUsage(const UiState& state)
{
    std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> usage{};
    auto addCosts = [&usage](const std::vector<EngineeringCost>& costs) {
        for (const auto& cost : costs) {
            usage[static_cast<std::size_t>(cost.domain)] += cost.amount;
        }
    };
    for (const auto& planned : state.plannedInterventions) {
        addCosts(planned.engineeringCosts);
    }
    addCosts(state.hoveredActionEngineeringCosts);
    return usage;
}

int distributedCapacityUsage(const EngineeringCapacity& capacity)
{
    return std::max(0, capacity.frontend)
        + std::max(0, capacity.backend)
        + std::max(0, capacity.infrastructure)
        + std::max(0, capacity.data);
}

const EngineeringCapacity& displayedCapacity(const UiState& state)
{
    return state.engineeringCapacityPreviewVisible ? state.previewEngineeringCapacity : state.engineeringCapacity;
}

const char* shortDomainName(EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend: return "Front";
    case EngineeringDomain::Backend: return "Back";
    case EngineeringDomain::Infrastructure: return "Infra";
    case EngineeringDomain::Data: return "Data";
    case EngineeringDomain::Count: break;
    }
    return "";
}

const char* domainIcon(EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend: return "engineering.frontend";
    case EngineeringDomain::Backend: return "engineering.backend";
    case EngineeringDomain::Infrastructure: return "engineering.infrastructure";
    case EngineeringDomain::Data: return "engineering.data";
    case EngineeringDomain::Count: break;
    }
    return "engineering.total";
}

void drawCapacitySquares(Vector2 pos, int used, int max, bool usedMeansFilled)
{
    constexpr float size = 10.0f;
    constexpr float gap = 4.0f;
    const int safeMax = std::max(0, max);
    const int safeUsed = std::clamp(used, 0, safeMax);

    for (int i = 0; i < safeMax; ++i) {
        const Rectangle square{pos.x + static_cast<float>(i) * (size + gap), pos.y, size, size};
        const bool active = usedMeansFilled ? i < safeUsed : i >= safeUsed;
        DrawRectangleRounded(square, 0.25f, 4, active ? Color{145, 109, 255, 255} : Color{44, 52, 64, 255});
        DrawRectangleRoundedLines(square, 0.25f, 4, active ? Color{189, 135, 255, 255} : Color{70, 86, 104, 120});
    }
}

void drawCapacityRow(Rectangle row, const char* iconId, const char* label, int used, int max, Color labelColor, bool usedMeansFilled)
{
    if (max <= 0) {
        return;
    }

    IconRegistry::instance().drawIcon(iconId, {row.x, row.y - 1.0f, 14.0f, 14.0f}, {139, 148, 158, 255});
    drawTextClipped(label, {row.x + 20.0f, row.y - 1.0f, 52.0f, 14.0f}, 11, labelColor);
    drawCapacitySquares({row.x + 78.0f, row.y}, used, max, usedMeansFilled);
}
}


float EngineeringCapacityPanel::measureHeight(const UiState& state) const
{
    constexpr float titleToFirstRow = 24.0f;
    constexpr float totalRowAdvance = 22.0f;
    constexpr float domainRowAdvance = 20.0f;
    constexpr float rowHeight = 14.0f;
    constexpr float bottomPadding = 4.0f;

    int visibleDomains = 0;
    for (int i = 0; i < static_cast<int>(EngineeringDomain::Count); ++i) {
        const auto domain = static_cast<EngineeringDomain>(i);
        if (capacityForDomain(displayedCapacity(state), domain) > 0) {
            ++visibleDomains;
        }
    }

    if (visibleDomains <= 0 && displayedCapacity(state).total <= 0) {
        return 18.0f;
    }

    return titleToFirstRow + totalRowAdvance +
        std::max(0, visibleDomains - 1) * domainRowAdvance + rowHeight + bottomPadding;
}

void EngineeringCapacityPanel::draw(Rectangle bounds, const UiState& state) const
{
    const auto usage = domainUsage(state);
    const EngineeringCapacity& capacity = displayedCapacity(state);

    drawTextClipped("ENGINEERING BUDGET", {bounds.x, bounds.y, bounds.width, 14.0f}, 11, {139, 148, 158, 255});

    float y = bounds.y + 24.0f;
    drawCapacityRow({bounds.x, y, bounds.width, 14.0f}, "engineering.total", "Budget", distributedCapacityUsage(capacity), capacity.total, {230, 237, 243, 255}, true);
    y += 22.0f;

    for (int i = 0; i < static_cast<int>(EngineeringDomain::Count); ++i) {
        const auto domain = static_cast<EngineeringDomain>(i);
        const int cap = capacityForDomain(capacity, domain);
        if (cap <= 0) {
            continue;
        }

        drawCapacityRow({bounds.x, y, bounds.width, 14.0f}, domainIcon(domain), shortDomainName(domain), usage[static_cast<std::size_t>(domain)], cap, {185, 195, 210, 255}, false);
        y += 20.0f;
    }
}
