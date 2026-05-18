#include "ui/actions/PressurePresentation.hpp"

#include <algorithm>

namespace actions_ui {

const char* pressureSeverity(double value)
{
    if (value >= 0.72) return "High";
    if (value >= 0.42) return "Medium";
    if (value > 0.08) return "Low";
    return "Calm";
}

Color pressureColor(double value)
{
    if (value >= 0.72) return {235, 86, 100, 255};
    if (value >= 0.42) return {245, 184, 76, 255};
    if (value > 0.08) return {89, 196, 255, 255};
    return {86, 210, 151, 255};
}

double overallPressure(const NodePressure* pressure, const Node* node)
{
    if (pressure == nullptr) {
        return node != nullptr ? node->currentUtilization : 0.0;
    }
    return std::max({pressure->queuePressure, pressure->computePressure, pressure->latencyContribution, pressure->timeoutContribution, pressure->retryContribution, pressure->dependencyPressure, pressure->instability});
}

}
