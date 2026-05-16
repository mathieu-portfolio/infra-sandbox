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
    }
    return "Unknown";
}
