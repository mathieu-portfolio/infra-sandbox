#pragma once

#include "simulation/Request.hpp"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

enum class NodeType {
    ClientCluster,
    Cache,
    Service,
    Database
};

enum class HealthState {
    Healthy,
    Saturated,
    Failing
};

struct Node {
    int id = -1;
    std::string name;
    NodeType type = NodeType::Service;
    Vec2 position{};
    double requestRatePerSecond = 0.0;
    double baseRequestRatePerSecond = 0.0;
    double processingCapacityPerSecond = 0.0;
    double baseProcessingCapacityPerSecond = 0.0;
    double timeoutSeconds = 6.0;
    double generationAccumulator = 0.0;
    double processingAccumulator = 0.0;
    double currentUtilization = 0.0;
    double averageQueueWaitSeconds = 0.0;
    HealthState health = HealthState::Healthy;
    std::deque<std::uint64_t> queue;
    std::vector<std::uint64_t> processing;

    [[nodiscard]] bool isProcessor() const;
};
