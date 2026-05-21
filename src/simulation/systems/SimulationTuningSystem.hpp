#pragma once
#include "gameplay/Scenario.hpp"
#include "core/simulation/Mechanics.hpp"
#include <optional>
#include <vector>
class Simulation;
class SimulationTuningSystem {
public:
    static void adjustClientRequestRates(Simulation& simulation, double deltaPerSecond);
    static void scaleApiCapacity(Simulation& simulation, double multiplier);
    static bool scaleApiCapacity(Simulation& simulation, int targetId, double multiplier, int maxScaleLevel, double diminishingReturn, double complexityCost);
    static void toggleCache(Simulation& simulation);
    static void clearCache(Simulation& simulation);
    static void toggleBurstMode(Simulation& simulation);
    static void toggleRetries(Simulation& simulation);
    static void resetProcessingCapacity(Simulation& simulation);
    static void setSimulationSpeed(Simulation& simulation, double speed);
    static void setScenarioTrafficMultiplier(Simulation& simulation, double multiplier);
    static void setScenarioBurst(Simulation& simulation, const BurstScenario& burst);
    static void setScenarioDatabaseCapacityMultiplier(Simulation& simulation, double multiplier);
    static void setScenarioLatencyMultiplier(Simulation& simulation, double multiplier);
    static void setScenarioDatabaseHeavyShareOverride(Simulation& simulation, std::optional<double> share);
    static void setScenarioRetryDelayMultiplier(Simulation& simulation, double multiplier);
    static void setLocalizedEventModifiers(Simulation& simulation, std::vector<LocalizedEventModifier> modifiers);
    static bool addRegionalDemandSource(Simulation& simulation, const EventLocation& location, double requestRatePerSecond);
    static void setScenarioTime(Simulation& simulation, double elapsedSeconds, double phaseElapsedSeconds, double calendarElapsedDays);
    static void clearScenarioBurstOverride(Simulation& simulation);
    static void setPaused(Simulation& simulation, bool paused);
    static void setAllowedMechanics(Simulation& simulation, const std::vector<MechanicType>& mechanics);
    static void addComplexity(Simulation& simulation, double amount);
    [[nodiscard]] static bool canScaleNode(const Simulation& simulation, int nodeId, int maxScaleLevel);
    [[nodiscard]] static int scaleLevelForNode(const Simulation& simulation, int nodeId);
    [[nodiscard]] static int maxScaleLevelForNode(const Simulation& simulation, int nodeId, int contentMaxScaleLevel);
};
