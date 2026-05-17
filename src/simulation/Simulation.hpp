#pragma once

#include "gameplay/Scenario.hpp"
#include "simulation/InfrastructureGraph.hpp"
#include "simulation/Metrics.hpp"
#include "simulation/PressureAnalysis.hpp"
#include "simulation/Request.hpp"
#include "simulation/RuntimeSystems.hpp"
#include "simulation/SimulationConfig.hpp"
#include "simulation/SimulationTime.hpp"

#include <cstdint>
#include <array>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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
    void setScenarioTime(double elapsedSeconds, double phaseElapsedSeconds);
    void setAllowedMechanics(const std::vector<MechanicType>& mechanics);
    void setPaused(bool paused);
    bool applyTopologyMutation(const struct TopologyMutation& mutation);
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
    struct CacheEntry {
        int key = 0;
        double expiresAt = 0.0;
    };

    void buildFromScenario(const ScenarioDefinition& scenario);
    void generateClientRequests(double dt);
    void createRequest(Node& clientNode, Link& link);
    void retryRequest(Request& request);
    void updateLinks(double dt);
    void updateProcessors(double dt);
    void updateRetryWaits();
    void updateTimeouts(Node& node);
    void enqueueAtNode(Request& request, Node& node);
    void completeRequest(Request& request, Node& node);
    void timeOutRequest(Request& request, Node& node);
    void timeOutRequest(Request& request);
    void routeFromApi(Request& request);
    void routeFromDatabase(Request& request);
    void routeToLink(Request& request, Link& link, RequestRouteStage nextStage);
    [[nodiscard]] Link* linkBetween(int sourceNodeId, int targetNodeId);
    [[nodiscard]] std::optional<int> firstNodeOfType(NodeType type) const;
    [[nodiscard]] double processingCost(const Request& request, const Node& node) const;
    [[nodiscard]] bool requestTimedOut(const Request& request) const;
    [[nodiscard]] bool cacheHit(int key);
    void storeCache(int key);
    void expireCacheEntries();
    void updateNodeHealth();
    void updateMetricsNodeStates();
    void refreshEffectiveCapacities();
    void refreshRegionSlots();
    void pruneOldRequests();

    InfrastructureGraph graph_;
    ScenarioDefinition scenario_;
    SimulationConfig config_;
    RuntimeSystems runtimeSystems_;
    SimulationTimeSystem timeSystem_;
    Metrics metrics_;
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
    std::array<bool, static_cast<std::size_t>(MechanicType::Count)> allowedMechanics_{};
    std::unordered_map<std::string, int> regionSlotsUsed_;
    std::unordered_map<std::string, int> regionSlotLimits_;
};
