#pragma once

#include "simulation/Geography.hpp"
#include "simulation/NodeDefinition.hpp"
#include "simulation/Request.hpp"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

enum class HealthState {
    Healthy,
    Saturated,
    Failing
};

struct Node {
    int id = -1;
    std::string name;
    NodeType type = NodeType::ApiService;
    Vec2 position{};
    GeoLocation geoLocation{};
    NetworkIdentity networkIdentity{};
    bool hasGeoLocation = false;
    double requestRatePerSecond = 0.0;
    double baseRequestRatePerSecond = 0.0;
    double processingCapacityPerSecond = 0.0;
    double baseProcessingCapacityPerSecond = 0.0;
    double mechanicCapacityMultiplier = 1.0;
    double eventCapacityMultiplier = 1.0;
    int scaleLevel = 0;
    int maxScaleLevel = 3;
    double timeoutSeconds = 6.0;
    double generationAccumulator = 0.0;
    double processingAccumulator = 0.0;
    double currentUtilization = 0.0;
    double averageQueueWaitSeconds = 0.0;

    // Utilization is service-capacity occupancy over a short rolling window.
    // It is intentionally separate from queue/backlog pressure: a node can be
    // busy without being overloaded, and can have backlog spikes without being
    // continuously saturated.
    double recentWorkConsumed = 0.0;
    double recentWorkCapacity = 0.0;

    // Continuous health model. HealthState is now only the coarse visual label
    // derived from these smoothed values.
    double healthScore = 1.0;       // 1.0 = healthy, 0.0 = unusable
    double stressScore = 0.0;       // blended local/dependency pressure
    double experienceScore = 1.0;   // user-perceived health for demand nodes
    double reliabilityScore = 1.0;  // recent success quality
    double queuePressure = 0.0;
    double latencyPressure = 0.0;
    double timeoutPressure = 0.0;
    double retryPressure = 0.0;

    // Recent request counters with exponential decay, used to avoid binary
    // health changes and to let client clusters become degraded when their
    // traffic experiences retries, timeouts, or long waits.
    double recentGenerated = 0.0;
    double recentCompleted = 0.0;
    double recentTimedOut = 0.0;
    double recentRetries = 0.0;

    HealthState health = HealthState::Healthy;
    std::deque<std::uint64_t> queue;
    std::vector<std::uint64_t> processing;

    [[nodiscard]] bool isProcessor() const;
};
