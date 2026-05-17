#include "ui/UiTypes.hpp"

const char* overlayModeName(OverlayMode mode)
{
    switch (mode) {
    case OverlayMode::None:
        return "None";
    case OverlayMode::Flow:
        return "Flow";
    case OverlayMode::Latency:
        return "Latency";
    case OverlayMode::Utilization:
        return "Utilization";
    case OverlayMode::Queues:
        return "Queues";
    case OverlayMode::Errors:
        return "Errors";
    case OverlayMode::Reliability:
        return "Reliability";
    case OverlayMode::Complexity:
        return "Complexity";
    case OverlayMode::Bottlenecks:
        return "Bottlenecks";
    case OverlayMode::RetryAmplification:
        return "Retry";
    }
    return "Unknown";
}

const char* timelineCategoryName(TimelineCategory category)
{
    switch (category) {
    case TimelineCategory::All:
        return "All Events";
    case TimelineCategory::Traffic:
        return "Traffic";
    case TimelineCategory::Change:
        return "Change";
    case TimelineCategory::Database:
        return "Database";
    case TimelineCategory::System:
        return "System";
    case TimelineCategory::Objectives:
        return "Objectives";
    case TimelineCategory::Reliability:
        return "Reliability";
    }
    return "Unknown";
}

const char* timelineFilterName(TimelineFilter filter)
{
    switch (filter) {
    case TimelineFilter::RecentFirst:
        return "Recent First";
    case TimelineFilter::OldestFirst:
        return "Oldest First";
    case TimelineFilter::ActiveOnly:
        return "Active Only";
    }
    return "Unknown";
}
