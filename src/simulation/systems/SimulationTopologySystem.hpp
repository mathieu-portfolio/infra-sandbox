#pragma once
#include <string>
class Simulation;
struct TopologyMutation;
class SimulationTopologySystem {
public:
    static bool applyTopologyMutation(Simulation& simulation, const TopologyMutation& mutation);
    [[nodiscard]] static bool canUseRegionSlots(const Simulation& simulation, const std::string& region, int slots);
    [[nodiscard]] static bool hasAnyRegionCapacity(const Simulation& simulation, int slots);
    [[nodiscard]] static int regionSlotsUsed(const Simulation& simulation, const std::string& region);
    [[nodiscard]] static int regionSlotLimit(const Simulation& simulation, const std::string& region);
    static void refreshRegionSlots(Simulation& simulation);
};
