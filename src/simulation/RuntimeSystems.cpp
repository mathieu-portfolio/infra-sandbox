#include "simulation/RuntimeSystems.hpp"

#include <array>

RuntimeSystemSkeleton::RuntimeSystemSkeleton(SimulationLayer layer, const char* name)
{
    state_.layer = layer;
    state_.name = name;
}

void RuntimeSystemSkeleton::initialize(const SimulationConfig& config)
{
    state_.enabled = config.isLayerEnabled(state_.layer);
    state_.initialized = true;
    state_.elapsedSeconds = 0.0;
}

void RuntimeSystemSkeleton::update(double dt)
{
    if (!state_.enabled) {
        return;
    }
    state_.elapsedSeconds += dt;
}

const RuntimeSystemState& RuntimeSystemSkeleton::state() const
{
    return state_;
}

std::string_view RuntimeSystemSkeleton::name() const
{
    return state_.name;
}

RequestFlowSystem::RequestFlowSystem() : RuntimeSystemSkeleton(SimulationLayer::Flow, "RequestFlowSystem") {}
QueueSystem::QueueSystem() : RuntimeSystemSkeleton(SimulationLayer::Flow, "QueueSystem") {}
LatencySystem::LatencySystem() : RuntimeSystemSkeleton(SimulationLayer::Flow, "LatencySystem") {}
CacheSystem::CacheSystem() : RuntimeSystemSkeleton(SimulationLayer::Persistence, "CacheSystem") {}
RetrySystem::RetrySystem() : RuntimeSystemSkeleton(SimulationLayer::Reliability, "RetrySystem") {}
FailureSystem::FailureSystem() : RuntimeSystemSkeleton(SimulationLayer::Reliability, "FailureSystem") {}
MetricsSystem::MetricsSystem() : RuntimeSystemSkeleton(SimulationLayer::Observability, "MetricsSystem") {}

void RuntimeSystems::initialize(const SimulationConfig& config)
{
    requestFlow_.initialize(config);
    queue_.initialize(config);
    latency_.initialize(config);
    cache_.initialize(config);
    retry_.initialize(config);
    failure_.initialize(config);
    metrics_.initialize(config);
    refreshStates();
}

void RuntimeSystems::update(double dt)
{
    requestFlow_.update(dt);
    queue_.update(dt);
    latency_.update(dt);
    cache_.update(dt);
    retry_.update(dt);
    failure_.update(dt);
    metrics_.update(dt);
    refreshStates();
}

std::span<const RuntimeSystemState> RuntimeSystems::states() const
{
    return states_;
}

int RuntimeSystems::enabledCount() const
{
    int count = 0;
    for (const auto& state : states_) {
        if (state.enabled) {
            ++count;
        }
    }
    return count;
}

void RuntimeSystems::refreshStates()
{
    states_[0] = requestFlow_.state();
    states_[1] = queue_.state();
    states_[2] = latency_.state();
    states_[3] = cache_.state();
    states_[4] = retry_.state();
    states_[5] = failure_.state();
    states_[6] = metrics_.state();
}
