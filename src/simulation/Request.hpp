#pragma once

#include <cstdint>

enum class RequestState {
    InTransit,
    Queued,
    Processing,
    Completed,
    TimedOut
};

struct Request {
    std::uint64_t id = 0;
    int sourceNodeId = -1;
    int targetNodeId = -1;
    int currentNodeId = -1;
    int currentLinkId = -1;
    RequestState state = RequestState::InTransit;
    double creationTime = 0.0;
    double stateEnteredTime = 0.0;
    double processingRemaining = 0.0;
    double transitProgress = 0.0;
    double completedTime = 0.0;
};
