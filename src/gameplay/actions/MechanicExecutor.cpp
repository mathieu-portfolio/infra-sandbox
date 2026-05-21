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

bool hasPressureEffect(const PressureState& effect)
{
    return effect.frontend.assetWeight != 0.0
        || effect.frontend.renderComplexity != 0.0
        || effect.frontend.cacheEfficiency != 0.0
        || effect.frontend.realtimeIntensity != 0.0
        || effect.frontend.sessionPersistence != 0.0
        || effect.frontend.mobileCompatibility != 0.0
        || effect.backend.requestLoad != 0.0
        || effect.backend.queuePressure != 0.0
        || effect.backend.computeIntensity != 0.0
        || effect.backend.serviceFragmentation != 0.0
        || effect.network.bandwidthPressure != 0.0
        || effect.network.latencySensitivity != 0.0
        || effect.network.trafficBurstiness != 0.0;
}
}

void MechanicExecutor::execute(Simulation& simulation, const MechanicCommand& command) const
{
    if (!simulation.isMechanicAllowed(command.type)) {
        return;
    }
    const auto* intervention = interventionFor(command.type);
    if (intervention != nullptr && hasPressureEffect(intervention->pressureEffect)) {
        simulation.applyPressureEffect(intervention->pressureEffect);
    }

    switch (command.type) {
    case MechanicType::ScaleUp:
        if (intervention != nullptr) {
            (void)simulation.scaleApiCapacity(command.targetId, command.amount > 0.0 ? command.amount : 1.5, intervention->maxScaleLevel, intervention->diminishingReturn, intervention->complexityCost);
        } else {
            simulation.scaleApiCapacity(command.amount > 0.0 ? command.amount : 1.5);
        }
        break;
    case MechanicType::EnableCache:
        simulation.toggleCache();
        if (intervention != nullptr) {
            // TODO: Keep legacy complexity costs until all action trade-offs are
            // expressed as typed hidden pressure effects.
            simulation.addComplexity(intervention->complexityCost);
        }
        break;
    case MechanicType::ClearCache:
        simulation.clearCache();
        break;
    case MechanicType::ToggleRetries:
        simulation.toggleRetries();
        if (intervention != nullptr) {
            // TODO: Keep legacy complexity costs until all action trade-offs are
            // expressed as typed hidden pressure effects.
            simulation.addComplexity(intervention->complexityCost);
        }
        break;
    case MechanicType::ThrottleTraffic:
        simulation.adjustClientRequestRates(command.amount);
        if (intervention != nullptr) {
            // TODO: Keep legacy complexity costs until all action trade-offs are
            // expressed as typed hidden pressure effects.
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
