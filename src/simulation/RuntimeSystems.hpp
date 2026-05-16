#pragma once

#include "simulation/SimulationConfig.hpp"

#include <array>
#include <span>
#include <string_view>

struct RuntimeSystemState {
    SimulationLayer layer = SimulationLayer::Flow;
    const char* name = "";
    bool enabled = true;
    bool initialized = false;
    double elapsedSeconds = 0.0;
};

class RuntimeSystemSkeleton {
public:
    RuntimeSystemSkeleton(SimulationLayer layer, const char* name);

    void initialize(const SimulationConfig& config);
    void update(double dt);

    [[nodiscard]] const RuntimeSystemState& state() const;
    [[nodiscard]] std::string_view name() const;

protected:
    RuntimeSystemState state_{};
};

class RequestFlowSystem : public RuntimeSystemSkeleton {
public:
    RequestFlowSystem();
};

class QueueSystem : public RuntimeSystemSkeleton {
public:
    QueueSystem();
};

class LatencySystem : public RuntimeSystemSkeleton {
public:
    LatencySystem();
};

class CacheSystem : public RuntimeSystemSkeleton {
public:
    CacheSystem();
};

class RetrySystem : public RuntimeSystemSkeleton {
public:
    RetrySystem();
};

class FailureSystem : public RuntimeSystemSkeleton {
public:
    FailureSystem();
};

class MetricsSystem : public RuntimeSystemSkeleton {
public:
    MetricsSystem();
};

class RuntimeSystems {
public:
    void initialize(const SimulationConfig& config);
    void update(double dt);

    [[nodiscard]] std::span<const RuntimeSystemState> states() const;
    [[nodiscard]] int enabledCount() const;

private:
    void refreshStates();

    RequestFlowSystem requestFlow_;
    QueueSystem queue_;
    LatencySystem latency_;
    CacheSystem cache_;
    RetrySystem retry_;
    FailureSystem failure_;
    MetricsSystem metrics_;
    std::array<RuntimeSystemState, 7> states_{};
};
