#pragma once

#include <cstdint>
#include <vector>

struct Link {
    int id = -1;
    int sourceNodeId = -1;
    int targetNodeId = -1;
    double baseLatencySeconds = 0.5;
    double bandwidthPerSecond = 100.0;
    std::vector<std::uint64_t> inFlightRequests;
};
