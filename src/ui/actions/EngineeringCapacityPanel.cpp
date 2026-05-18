#include "ui/actions/EngineeringCapacityPanel.hpp"

#include "ui/IconRegistry.hpp"
#include "ui/core/UiPrimitives.hpp"

#include <algorithm>
#include <array>

namespace {
int capacityForDomain(const EngineeringCapacity& capacity, EngineeringDomain domain)
{
    switch (domain) {
    case EngineeringDomain::Frontend: return capacity.frontend;
    case EngineeringDomain::Backend: return capacity.backend;
    case EngineeringDomain::Infrastructure: return capacity.infrastructure;
    case EngineeringDomain::Data: return capacity.data;
    case EngineeringDomain::Operations: return capacity.operations;
    case EngineeringDomain::Count: break;
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
        const Rectangle square{pos.x + static_cast<float>(i) * (size + gap), pos.y, size, size};
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
}

void EngineeringCapacityPanel::draw(Rectangle bounds, const UiState& state) const
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

        drawCapacityRow({bounds.x, y, bounds.width, 14.0f}, domainIcon(domain), shortDomainName(domain), usage[static_cast<std::size_t>(domain)], cap, {185, 195, 210, 255});
        y += 20.0f;
    }
}
