#include "ui/core/UiTypes.hpp"


const char* uiViewModeName(UiViewMode mode)
{
    switch (mode) {
    case UiViewMode::Overview:
        return "Overview";
    case UiViewMode::Traffic:
        return "Traffic";
    case UiViewMode::Resources:
        return "Resources";
    case UiViewMode::Persistence:
        return "Persistence";
    case UiViewMode::Reliability:
        return "Reliability";
    case UiViewMode::Geography:
        return "Geography";
    case UiViewMode::Count:
        break;
    }
    return "Unknown";
}

const char* uiViewModeIcon(UiViewMode mode)
{
    switch (mode) {
    case UiViewMode::Overview:
        return "view.overview";
    case UiViewMode::Traffic:
        return "layer.flow";
    case UiViewMode::Resources:
        return "layer.resources";
    case UiViewMode::Persistence:
        return "layer.persistence";
    case UiViewMode::Reliability:
        return "layer.reliability";
    case UiViewMode::Geography:
        return "layer.geography";
    case UiViewMode::Count:
        break;
    }
    return "view.overview";
}

OverlayMode overlayForViewMode(UiViewMode mode)
{
    switch (mode) {
    case UiViewMode::Overview:
        return OverlayMode::Bottlenecks;
    case UiViewMode::Traffic:
        return OverlayMode::Flow;
    case UiViewMode::Resources:
        return OverlayMode::Utilization;
    case UiViewMode::Persistence:
        return OverlayMode::Queues;
    case UiViewMode::Reliability:
        return OverlayMode::Reliability;
    case UiViewMode::Geography:
        return OverlayMode::Latency;
    case UiViewMode::Count:
        break;
    }
    return OverlayMode::None;
}

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
