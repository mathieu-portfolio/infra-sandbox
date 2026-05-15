#pragma once

#include <cstdint>

enum class RequestState {
    InTransit,
    Queued,
    Processing,
    RetryWaiting,
    Completed,
    TimedOut
};

enum class RequestType {
    Lightweight,
    DatabaseHeavy
};

enum class RequestRouteStage {
    ToApi,
    ApiIngress,
    ToDatabase,
    DatabaseWork,
    BackToApi,
    ApiReturn
};

struct Request {
    std::uint64_t id = 0;
    int sourceNodeId = -1;
    int targetNodeId = -1;
    int currentNodeId = -1;
    int currentLinkId = -1;
    RequestState state = RequestState::InTransit;
    RequestType type = RequestType::Lightweight;
    RequestRouteStage routeStage = RequestRouteStage::ToApi;
    double creationTime = 0.0;
    double stateEnteredTime = 0.0;
    double processingRemaining = 0.0;
    double transitProgress = 0.0;
    double completedTime = 0.0;
    double retryDueTime = 0.0;
    int retryCount = 0;
    int cacheKey = 0;
    bool cacheable = false;
    bool servedFromCache = false;
};
