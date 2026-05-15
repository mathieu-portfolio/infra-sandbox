#pragma once

#include "gameplay/Scenario.hpp"
#include "simulation/InfrastructureGraph.hpp"
#include "simulation/Metrics.hpp"
#include "simulation/Request.hpp"

#include <cstdint>
#include <unordered_map>

class Simulation {
public:
    explicit Simulation(const ScenarioDefinition& scenario);

    void update(double dt);
    void adjustClientRequestRates(double deltaPerSecond);
    void scaleProcessingCapacity(double multiplier);
    void applyCachePlaceholder();
    void resetProcessingCapacity();

    [[nodiscard]] const InfrastructureGraph& graph() const;
    [[nodiscard]] const std::unordered_map<std::uint64_t, Request>& requests() const;
    [[nodiscard]] const MetricsSnapshot& metrics() const;
    [[nodiscard]] double timeSeconds() const;
    [[nodiscard]] double cacheEfficiency() const;

private:
    void buildFromScenario(const ScenarioDefinition& scenario);
    void generateClientRequests(double dt);
    void createRequest(Node& clientNode, Link& link);
    void updateLinks(double dt);
    void updateProcessors(double dt);
    void updateTimeouts(Node& node);
    void completeRequest(Request& request, Node& node);
    void timeOutRequest(Request& request, Node& node);
    void updateNodeHealth();
    void pruneOldRequests();

    InfrastructureGraph graph_;
    Metrics metrics_;
    std::unordered_map<std::uint64_t, Request> requests_;
    std::uint64_t nextRequestId_ = 1;
    double timeSeconds_ = 0.0;
    double cacheEfficiency_ = 0.0;
};
