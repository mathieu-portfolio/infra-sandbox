#include "gameplay/actions/ActionFeedback.hpp"

#include "gameplay/actions/ActionRules.hpp"
#include "simulation/core/Simulation.hpp"

namespace gameplay::actions {

std::string feedbackForMechanic(MechanicType mechanic, const std::string& fallbackActionName)
{
    std::string message = fallbackActionName + " applied.";
    if (const auto* intervention = interventionFor(mechanic); intervention != nullptr && !intervention->positiveEffects.empty()) {
        message = intervention->positiveEffects.front() + ".";
        if (!intervention->pressureShifts.empty()) {
            message += " " + intervention->pressureShifts.front() + ".";
        }
    } else if (mechanic == MechanicType::ScaleUp) {
        message = "API capacity increased. Watch queue depth and utilization.";
    } else if (mechanic == MechanicType::ToggleRetries) {
        message = "Retry policy changed. Watch timeout rate and retry amplification.";
    } else if (mechanic == MechanicType::ClearCache) {
        message = "Cache cleared. Repeated reads may warm it again.";
    } else if (mechanic == MechanicType::EnableCache) {
        message = "Cache behavior toggled. Watch cache hit rate and DB pressure.";
    }
    return message;
}

std::string feedbackForTopologyMutation(TopologyMutationType mutationType, const std::string& fallbackTarget)
{
    const MechanicType mechanic = mechanicForMutation(mutationType);
    std::string feedback = std::string(topologyMutationName(mutationType)) + " queued in " + fallbackTarget + ". Resolve the turn to see consequences.";
    if (const auto* intervention = interventionFor(mechanic); intervention != nullptr) {
        if (!intervention->positiveEffects.empty()) {
            feedback = intervention->positiveEffects.front() + ".";
        }
        if (!intervention->pressureShifts.empty()) {
            feedback += " " + intervention->pressureShifts.front() + ".";
        }
    }
    return feedback;
}

void recordActionFeedback(UiState& uiState, const Simulation& simulation, std::string actionName, std::string target, std::string message)
{
    uiState.latestFeedback = message;
    uiState.actionHistory.push_back({simulation.timeSeconds(), std::move(actionName), std::move(target), message, simulation.metrics(), true, false, 4.0});
    while (uiState.actionHistory.size() > 8) {
        uiState.actionHistory.pop_front();
    }
}

}
