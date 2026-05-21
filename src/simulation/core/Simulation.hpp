#pragma once

#include "gameplay/Scenario.hpp"
#include "simulation/topology/InfrastructureGraph.hpp"
#include "simulation/metrics/Metrics.hpp"
#include "simulation/metrics/PressureAnalysis.hpp"
#include "simulation/requests/Request.hpp"
#include "simulation/core/RuntimeSystems.hpp"
#include "simulation/core/SimulationConfig.hpp"
#include "simulation/core/SimulationTime.hpp"

#include <cstdint>
#include <array>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


class SimulationRequestFlowSystem;
class SimulationHealthSystem;
class SimulationModifierSystem;
class SimulationPressureSystem;
class SimulationTopologySystem;
class SimulationTuningSystem;
class SimulationScenarioBuilder;

class Simulation {
public:
    explicit Simulation(const ScenarioDefinition& scenario, SimulationConfig config = {});

    void update(double dt);
    void adjustClientRequestRates(double deltaPerSecond);
    void scaleApiCapacity(double multiplier);
    bool scaleApiCapacity(int targetId, double multiplier, int maxScaleLevel, double diminishingReturn, double complexityCost);
    void toggleCache();
    void clearCache();
    void toggleBurstMode();
    void toggleRetries();
    void resetProcessingCapacity();
    void setSimulationSpeed(double speed);
    void setScenarioTrafficMultiplier(double multiplier);
    void setScenarioBurst(const BurstScenario& burst);
    void clearScenarioBurstOverride();
    void setScenarioDatabaseCapacityMultiplier(double multiplier);
    void setScenarioLatencyMultiplier(double multiplier);
    void setScenarioDatabaseHeavyShareOverride(std::optional<double> share);
    void setScenarioRetryDelayMultiplier(double multiplier);
    void setLocalizedEventModifiers(std::vector<LocalizedEventModifier> modifiers);
    bool addRegionalDemandSource(const EventLocation& location, double requestRatePerSecond);
    void setScenarioTime(double elapsedSeconds, double phaseElapsedSeconds, double calendarElapsedDays = 0.0);
    void setAllowedMechanics(const std::vector<MechanicType>& mechanics);
    void setPaused(bool paused);
    bool applyTopologyMutation(const struct TopologyMutation& mutation);
    void applyPressureEffect(const PressureState& effect);
    void setEventPressureContext(const PressureState& context, std::vector<PressureContextSignal> signals);
    void addComplexity(double amount);
    bool canScaleNode(int nodeId, int maxScaleLevel) const;
    int scaleLevelForNode(int nodeId) const;
    int maxScaleLevelForNode(int nodeId, int contentMaxScaleLevel) const;
    bool canUseRegionSlots(const std::string& region, int slots) const;
    bool hasAnyRegionCapacity(int slots) const;
    int regionSlotsUsed(const std::string& region) const;
    int regionSlotLimit(const std::string& region) const;

    [[nodiscard]] const InfrastructureGraph& graph() const;
    [[nodiscard]] const std::unordered_map<std::uint64_t, Request>& requests() const;
    [[nodiscard]] const MetricsSnapshot& metrics() const;
    [[nodiscard]] const PressureSnapshot& pressure() const;
    [[nodiscard]] const PressureAnalysisSystem& pressureAnalysis() const;
    [[nodiscard]] double timeSeconds() const;
    [[nodiscard]] bool cacheEnabled() const;
    [[nodiscard]] bool burstModeEnabled() const;
    [[nodiscard]] bool retriesEnabled() const;
    [[nodiscard]] double simulationSpeed() const;
    [[nodiscard]] bool isMechanicAllowed(MechanicType mechanic) const;
    [[nodiscard]] const TimeState& timeState() const;
    [[nodiscard]] const RuntimeSystems& runtimeSystems() const;
    [[nodiscard]] const SimulationConfig& config() const;
    [[nodiscard]] double complexityScore() const;
    [[nodiscard]] double recommendedComplexityThreshold() const;

private:
    friend class SimulationRequestFlowSystem;
    friend class SimulationHealthSystem;
    friend class SimulationModifierSystem;
    friend class SimulationPressureSystem;
    friend class SimulationTopologySystem;
    friend class SimulationTuningSystem;
    friend class SimulationScenarioBuilder;

    struct CacheEntry {
        int key = 0;
        double expiresAt = 0.0;
    };

    InfrastructureGraph graph_;
    ScenarioDefinition scenario_;
    SimulationConfig config_;
    RuntimeSystems runtimeSystems_;
    SimulationTimeSystem timeSystem_;
    Metrics metrics_;
    PressureState pressureState_;
    PressureState scenarioPressureContext_;
    PressureState eventPressureContext_;
    std::vector<PressureContextSignal> scenarioPressureSignals_;
    std::vector<PressureContextSignal> eventPressureSignals_;
    PressureAnalysisSystem pressureAnalysis_;
    std::unordered_map<std::uint64_t, Request> requests_;
    std::deque<CacheEntry> cacheEntries_;
    std::uint64_t nextRequestId_ = 1;
    double timeSeconds_ = 0.0;
    double simulationSpeed_ = 1.0;
    double scenarioTrafficMultiplier_ = 1.0;
    double scenarioDatabaseCapacityMultiplier_ = 1.0;
    double scenarioRetryDelayMultiplier_ = 1.0;
    double scenarioLatencyMultiplier_ = 1.0;
    double complexityScore_ = 0.0;
    double recommendedComplexityThreshold_ = 10.0;
    bool cacheEnabled_ = false;
    bool burstModeEnabled_ = false;
    std::optional<BurstScenario> scenarioBurstOverride_;
    std::optional<double> scenarioDatabaseHeavyShareOverride_;
    std::vector<LocalizedEventModifier> localizedEventModifiers_;
    std::array<bool, static_cast<std::size_t>(MechanicType::Count)> allowedMechanics_{};
    std::unordered_map<std::string, int> regionSlotsUsed_;
    std::unordered_map<std::string, int> regionSlotLimits_;
};
