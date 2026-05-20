#include "core/simulation/Mechanics.hpp"

#include "content/ContentRegistry.hpp"
#include "simulation/core/Simulation.hpp"

namespace {
const content::InterventionDefinition* interventionFor(MechanicType mechanic)
{
    for (const auto& definition : content::ContentRegistry::instance().interventions()) {
        if (definition.mechanic == mechanic) {
            return &definition;
        }
    }
    return nullptr;
}
}

void MechanicExecutor::execute(Simulation& simulation, const MechanicCommand& command) const
{
    if (!simulation.isMechanicAllowed(command.type)) {
        return;
    }

    switch (command.type) {
    case MechanicType::ScaleUp:
        if (const auto* intervention = interventionFor(command.type)) {
            (void)simulation.scaleApiCapacity(command.targetId, command.amount > 0.0 ? command.amount : 1.5, intervention->maxScaleLevel, intervention->diminishingReturn, intervention->complexityCost);
        } else {
            simulation.scaleApiCapacity(command.amount > 0.0 ? command.amount : 1.5);
        }
        break;
    case MechanicType::EnableCache:
        simulation.toggleCache();
        if (const auto* intervention = interventionFor(command.type)) {
            simulation.addComplexity(intervention->complexityCost);
        }
        break;
    case MechanicType::ClearCache:
        simulation.clearCache();
        break;
    case MechanicType::ToggleRetries:
        simulation.toggleRetries();
        if (const auto* intervention = interventionFor(command.type)) {
            simulation.addComplexity(intervention->complexityCost);
        }
        break;
    case MechanicType::ThrottleTraffic:
        simulation.adjustClientRequestRates(command.amount);
        if (const auto* intervention = interventionFor(command.type)) {
            simulation.addComplexity(intervention->complexityCost);
        }
        break;
    case MechanicType::ScaleOut:
    case MechanicType::AdjustRetryPolicy:
    case MechanicType::AddCache:
    case MechanicType::AddQueue:
    case MechanicType::AddLoadBalancer:
    case MechanicType::AddReadReplica:
    case MechanicType::AddRegionalCache:
    case MechanicType::SplitService:
    case MechanicType::EnableTracing:
    case MechanicType::Count:
        break;
    }
}
