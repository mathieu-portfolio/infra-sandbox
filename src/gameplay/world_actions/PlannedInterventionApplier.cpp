#include "gameplay/world_actions/PlannedInterventionApplier.hpp"

#include "core/simulation/Mechanics.hpp"
#include "gameplay/world_actions/EngineeringCapacityUsage.hpp"
#include "simulation/topology/TopologyMutation.hpp"

namespace {
MechanicType mechanicForMutation(TopologyMutationType mutationType)
{
    return mutationType == TopologyMutationType::AddCache ? MechanicType::AddCache
        : mutationType == TopologyMutationType::AddReadReplica ? MechanicType::AddReadReplica
        : mutationType == TopologyMutationType::AddQueue ? MechanicType::AddQueue
        : MechanicType::AddRegionalCache;
}
}

void PlannedInterventionApplier::apply(UiState& state, ScenarioSession& session) const
{
    MechanicExecutor executor;
    TopologyBuilder topologyBuilder;
    const std::string worldActionSummary = state.lastCapacityUsageSummary;
    state.lastCapacityUsageSummary = gameplay::world_actions::capacityUsageSummary(state);
    if (!worldActionSummary.empty()) {
        state.lastCapacityUsageSummary = state.lastCapacityUsageSummary.empty()
            ? worldActionSummary
            : state.lastCapacityUsageSummary + "; " + worldActionSummary;
    }
    for (const auto& planned : state.plannedInterventions) {
        if (planned.kind == PlannedInterventionKind::Mechanic) {
            executor.execute(session.simulation(), planned.command);
            session.scenarioManager().notifyActionTriggered(planned.command.type);
            state.pendingVisualFeedbackEvents.push_back({
                .kind = planned.command.type == MechanicType::ThrottleTraffic ? VisualFeedbackKind::TrafficShift : VisualFeedbackKind::ActionAcknowledged,
                .targetNodeId = planned.command.targetId,
                .mechanic = planned.command.type,
                .label = planned.actionName,
            });
        } else {
            if (topologyBuilder.apply(session.simulation(), planned.mutation)) {
                const MechanicType mechanic = mechanicForMutation(planned.mutationType);
                session.scenarioManager().notifyActionTriggered(mechanic);
                state.pendingVisualFeedbackEvents.push_back({
                    .kind = VisualFeedbackKind::TopologyMutation,
                    .targetNodeId = -1,
                    .targetLinkId = -1,
                    .mechanic = mechanic,
                    .mutation = planned.mutationType,
                    .label = planned.actionName,
                });
            }
        }
        state.actionHistory.push_back({
            session.simulation().timeSeconds(),
            planned.actionId,
            planned.actionName,
            planned.target,
            planned.preview,
            session.simulation().metrics(),
            true,
            false,
            4.0,
        });
    }
    state.plannedInterventions.clear();
}
