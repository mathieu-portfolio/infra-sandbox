#include "simulation/core/Simulation.hpp"

#include "simulation/systems/SimulationPressureSystem.hpp"
#include "simulation/systems/SimulationTopologySystem.hpp"
#include "simulation/systems/SimulationTuningSystem.hpp"

void Simulation::adjustClientRequestRates(double deltaPerSecond)
{
    SimulationTuningSystem::adjustClientRequestRates(*this, deltaPerSecond);
}

void Simulation::scaleApiCapacity(double multiplier)
{
    SimulationTuningSystem::scaleApiCapacity(*this, multiplier);
}

bool Simulation::scaleApiCapacity(int targetId, double multiplier, int maxScaleLevel, double diminishingReturn, double complexityCost)
{
    return SimulationTuningSystem::scaleApiCapacity(*this, targetId, multiplier, maxScaleLevel, diminishingReturn, complexityCost);
}

void Simulation::toggleCache()
{
    SimulationTuningSystem::toggleCache(*this);
}

void Simulation::clearCache()
{
    SimulationTuningSystem::clearCache(*this);
}

void Simulation::toggleBurstMode()
{
    SimulationTuningSystem::toggleBurstMode(*this);
}

void Simulation::toggleRetries()
{
    SimulationTuningSystem::toggleRetries(*this);
}

void Simulation::resetProcessingCapacity()
{
    SimulationTuningSystem::resetProcessingCapacity(*this);
}

void Simulation::setSimulationSpeed(double speed)
{
    SimulationTuningSystem::setSimulationSpeed(*this, speed);
}

void Simulation::setScenarioTrafficMultiplier(double multiplier)
{
    SimulationTuningSystem::setScenarioTrafficMultiplier(*this, multiplier);
}

void Simulation::setScenarioBurst(const BurstScenario& burst)
{
    SimulationTuningSystem::setScenarioBurst(*this, burst);
}

void Simulation::clearScenarioBurstOverride()
{
    SimulationTuningSystem::clearScenarioBurstOverride(*this);
}

void Simulation::setScenarioDatabaseCapacityMultiplier(double multiplier)
{
    SimulationTuningSystem::setScenarioDatabaseCapacityMultiplier(*this, multiplier);
}

void Simulation::setScenarioLatencyMultiplier(double multiplier)
{
    SimulationTuningSystem::setScenarioLatencyMultiplier(*this, multiplier);
}

void Simulation::setScenarioDatabaseHeavyShareOverride(std::optional<double> share)
{
    SimulationTuningSystem::setScenarioDatabaseHeavyShareOverride(*this, share);
}

void Simulation::setScenarioRetryDelayMultiplier(double multiplier)
{
    SimulationTuningSystem::setScenarioRetryDelayMultiplier(*this, multiplier);
}

void Simulation::setLocalizedEventModifiers(std::vector<LocalizedEventModifier> modifiers)
{
    SimulationTuningSystem::setLocalizedEventModifiers(*this, std::move(modifiers));
}

bool Simulation::addRegionalDemandSource(const EventLocation& location, double requestRatePerSecond)
{
    return SimulationTuningSystem::addRegionalDemandSource(*this, location, requestRatePerSecond);
}

void Simulation::setScenarioTime(double elapsedSeconds, double phaseElapsedSeconds, double calendarElapsedDays)
{
    SimulationTuningSystem::setScenarioTime(*this, elapsedSeconds, phaseElapsedSeconds, calendarElapsedDays);
}

void Simulation::setAllowedMechanics(const std::vector<MechanicType>& mechanics)
{
    SimulationTuningSystem::setAllowedMechanics(*this, mechanics);
}

void Simulation::setPaused(bool paused)
{
    SimulationTuningSystem::setPaused(*this, paused);
}

bool Simulation::applyTopologyMutation(const TopologyMutation& mutation)
{
    return SimulationTopologySystem::applyTopologyMutation(*this, mutation);
}

void Simulation::applyPressureEffect(const PressureState& effect)
{
    SimulationPressureSystem::applyPressureEffect(*this, effect);
}

void Simulation::setEventPressureContext(const PressureState& context, std::vector<PressureContextSignal> signals)
{
    SimulationPressureSystem::setEventPressureContext(*this, context, std::move(signals));
}

void Simulation::addComplexity(double amount)
{
    SimulationTuningSystem::addComplexity(*this, amount);
}

bool Simulation::canScaleNode(int nodeId, int maxScaleLevel) const
{
    return SimulationTuningSystem::canScaleNode(*this, nodeId, maxScaleLevel);
}

int Simulation::scaleLevelForNode(int nodeId) const
{
    return SimulationTuningSystem::scaleLevelForNode(*this, nodeId);
}

int Simulation::maxScaleLevelForNode(int nodeId, int contentMaxScaleLevel) const
{
    return SimulationTuningSystem::maxScaleLevelForNode(*this, nodeId, contentMaxScaleLevel);
}

bool Simulation::canUseRegionSlots(const std::string& region, int slots) const
{
    return SimulationTopologySystem::canUseRegionSlots(*this, region, slots);
}

bool Simulation::hasAnyRegionCapacity(int slots) const
{
    return SimulationTopologySystem::hasAnyRegionCapacity(*this, slots);
}

int Simulation::regionSlotsUsed(const std::string& region) const
{
    return SimulationTopologySystem::regionSlotsUsed(*this, region);
}

int Simulation::regionSlotLimit(const std::string& region) const
{
    return SimulationTopologySystem::regionSlotLimit(*this, region);
}
