#include "gameplay/world_actions/EngineeringCapacityUsage.hpp"

#include "core/scenario/ScenarioEnums.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace gameplay::world_actions {

std::string capacityUsageSummary(const UiState& state)
{
    std::array<int, static_cast<std::size_t>(EngineeringDomain::Count)> domainUsage{};
    int total = 0;
    for (const auto& planned : state.plannedInterventions) {
        for (const auto& cost : planned.engineeringCosts) {
            domainUsage[static_cast<std::size_t>(cost.domain)] += cost.amount;
            total += cost.amount;
        }
    }
    if (total == 0) {
        return "0/" + std::to_string(state.engineeringCapacity.total);
    }

    std::string summary = std::to_string(total) + "/" + std::to_string(state.engineeringCapacity.total) + " budget";
    for (std::size_t index = 0; index < domainUsage.size(); ++index) {
        if (domainUsage[index] <= 0) {
            continue;
        }
        const auto domain = static_cast<EngineeringDomain>(index);
        summary += ", ";
        summary += engineeringDomainName(domain);
        summary += " ";
        summary += std::to_string(domainUsage[index]);
    }
    return summary;
}

}
