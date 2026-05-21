#include "content/loading/ContentEnumParsers.hpp"

#include <set>

namespace content::loading {

ProgressionTier progressionTierFromId(const std::string& id)
{
    if (id == "foundations") return ProgressionTier::Foundations;
    if (id == "local_scale") return ProgressionTier::LocalScale;
    if (id == "state_and_cache") return ProgressionTier::StateAndCache;
    if (id == "failure_feedback") return ProgressionTier::FailureFeedback;
    if (id == "geographic_scale") return ProgressionTier::GeographicScale;
    if (id == "distributed_systems") return ProgressionTier::DistributedSystems;
    if (id == "complexity") return ProgressionTier::Complexity;
    return ProgressionTier::Foundations;
}

ScenarioArchetype archetypeFromId(const std::string& id)
{
    if (id == "first_request") return ScenarioArchetype::FirstRequest;
    if (id == "local_startup") return ScenarioArchetype::LocalStartup;
    if (id == "database_bottleneck" || id == "database_pressure") return ScenarioArchetype::DatabasePressure;
    if (id == "burst_traffic") return ScenarioArchetype::BurstTraffic;
    if (id == "transatlantic_latency") return ScenarioArchetype::TransatlanticLatency;
    if (id == "global_read_platform") return ScenarioArchetype::GlobalReadPlatform;
    return ScenarioArchetype::LocalStartup;
}

GameplayDurationUnit durationUnitFromId(const std::string& id)
{
    if (id == "turn" || id == "turns" || id == "operational_cycle" || id == "operational_cycles") return GameplayDurationUnit::Turns;
    if (id == "minute" || id == "minutes") return GameplayDurationUnit::Minutes;
    if (id == "day" || id == "days") return GameplayDurationUnit::Days;
    if (id == "month" || id == "months") return GameplayDurationUnit::Months;
    if (id == "year" || id == "years") return GameplayDurationUnit::Years;
    return GameplayDurationUnit::Seconds;
}

EducationalFocus focusFromId(const std::string& id)
{
    if (id == "queues") return EducationalFocus::Queues;
    if (id == "caching") return EducationalFocus::Caching;
    if (id == "scaling") return EducationalFocus::Scaling;
    if (id == "reliability") return EducationalFocus::Reliability;
    if (id == "persistence") return EducationalFocus::Persistence;
    if (id == "latency") return EducationalFocus::Latency;
    if (id == "geography") return EducationalFocus::Geography;
    return EducationalFocus::Scaling;
}

PressureCategory pressureFromId(const std::string& id)
{
    if (id == "traffic") return PressureCategory::TrafficPressure;
    if (id == "queue") return PressureCategory::QueuePressure;
    if (id == "compute") return PressureCategory::ComputePressure;
    if (id == "persistence") return PressureCategory::PersistencePressure;
    if (id == "retry") return PressureCategory::RetryPressure;
    if (id == "latency") return PressureCategory::LatencyPressure;
    if (id == "failure") return PressureCategory::FailurePressure;
    return PressureCategory::None;
}

MechanicType mechanicFromId(const std::string& id)
{
    if (id == "scale_up") return MechanicType::ScaleUp;
    if (id == "scale_out") return MechanicType::ScaleOut;
    if (id == "enable_cache") return MechanicType::EnableCache;
    if (id == "clear_cache") return MechanicType::ClearCache;
    if (id == "toggle_retries") return MechanicType::ToggleRetries;
    if (id == "add_cache") return MechanicType::AddCache;
    if (id == "add_queue") return MechanicType::AddQueue;
    if (id == "add_load_balancer") return MechanicType::AddLoadBalancer;
    if (id == "add_read_replica") return MechanicType::AddReadReplica;
    if (id == "add_regional_cache") return MechanicType::AddRegionalCache;
    if (id == "throttle_traffic") return MechanicType::ThrottleTraffic;
    if (id == "enable_tracing") return MechanicType::EnableTracing;
    return MechanicType::ScaleUp;
}

bool knownMechanicId(const std::string& id)
{
    static const std::set<std::string> ids{
        "scale_up", "scale_out", "enable_cache", "clear_cache", "toggle_retries", "add_cache",
        "add_queue", "add_load_balancer", "add_read_replica", "add_regional_cache",
        "throttle_traffic", "enable_tracing",
    };
    return ids.contains(id);
}

TopologyMutationType mutationFromId(const std::string& id)
{
    if (id == "add_read_replica") return TopologyMutationType::AddReadReplica;
    if (id == "add_queue") return TopologyMutationType::AddQueue;
    if (id == "add_regional_cache") return TopologyMutationType::AddRegionalCache;
    return TopologyMutationType::AddCache;
}

bool knownMutationId(const std::string& id)
{
    static const std::set<std::string> ids{"add_cache", "add_read_replica", "add_queue", "add_regional_cache"};
    return ids.contains(id);
}

NodeType nodeTypeFromId(const std::string& id)
{
    if (id == "client_cluster") return NodeType::ClientCluster;
    if (id == "api_service") return NodeType::ApiService;
    if (id == "microservice") return NodeType::Microservice;
    if (id == "worker") return NodeType::Worker;
    if (id == "batch_processor") return NodeType::BatchProcessor;
    if (id == "stream_processor") return NodeType::StreamProcessor;
    if (id == "database") return NodeType::Database;
    if (id == "cache") return NodeType::Cache;
    if (id == "read_replica") return NodeType::ReadReplica;
    if (id == "queue") return NodeType::QueueBroker;
    if (id == "cdn_edge") return NodeType::CDNEdge;
    if (id == "load_balancer") return NodeType::LoadBalancer;
    return NodeType::ApiService;
}

bool knownNodeTypeId(const std::string& id)
{
    static const std::set<std::string> ids{
        "client_cluster", "api_service", "microservice", "worker", "batch_processor", "stream_processor",
        "database", "cache", "read_replica", "queue", "cdn_edge", "load_balancer",
    };
    return ids.contains(id);
}

EngineeringDomain engineeringDomainFromId(const std::string& id)
{
    if (id == "frontend") return EngineeringDomain::Frontend;
    if (id == "infrastructure" || id == "infra") return EngineeringDomain::Infrastructure;
    if (id == "data") return EngineeringDomain::Data;
    return EngineeringDomain::Backend;
}

bool knownEngineeringDomainId(const std::string& id)
{
    static const std::set<std::string> ids{"frontend", "backend", "infrastructure", "infra", "data"};
    return ids.contains(id);
}

TrafficProfileType trafficTypeFromId(const std::string& id)
{
    if (id == "gradual_growth") return TrafficProfileType::GradualGrowth;
    if (id == "bursty") return TrafficProfileType::Bursty;
    if (id == "periodic_spike") return TrafficProfileType::PeriodicSpikes;
    if (id == "regional_growth") return TrafficProfileType::GradualGrowth;
    return TrafficProfileType::Constant;
}

ScenarioObjectiveType objectiveTypeFromId(const std::string& id)
{
    if (id == "metric_threshold" || id == "max_latency") return ScenarioObjectiveType::MaxLatency;
    if (id == "max_error_rate") return ScenarioObjectiveType::MaxErrorRate;
    if (id == "min_throughput") return ScenarioObjectiveType::MinThroughput;
    if (id == "stabilize_metric" || id == "stabilize_queues") return ScenarioObjectiveType::StabilizeQueues;
    if (id == "recover_after_event") return ScenarioObjectiveType::StabilizeQueues;
    return ScenarioObjectiveType::SurviveDuration;
}

ObjectiveConditionType objectiveConditionFromId(const std::string& id)
{
    if (id == "pressure_detected") return ObjectiveConditionType::PressureDetected;
    if (id == "pressure_below") return ObjectiveConditionType::PressureBelow;
    if (id == "metric_below") return ObjectiveConditionType::MetricBelow;
    if (id == "action_used") return ObjectiveConditionType::ActionUsed;
    return ObjectiveConditionType::SurviveDuration;
}

ObjectiveRewardType objectiveRewardFromId(const std::string& id)
{
    if (id == "unlock_action" || id == "unlock_intervention") return ObjectiveRewardType::UnlockIntervention;
    if (id == "unlock_metric") return ObjectiveRewardType::UnlockMetric;
    if (id == "unlock_overlay") return ObjectiveRewardType::UnlockOverlay;
    if (id == "unlock_scenario_phase") return ObjectiveRewardType::UnlockScenarioPhase;
    if (id == "unlock_scenario") return ObjectiveRewardType::UnlockScenario;
    if (id == "complete_scenario") return ObjectiveRewardType::CompleteScenario;
    return ObjectiveRewardType::EmitFeedback;
}

EventCategory eventCategoryFromId(const std::string& id)
{
    if (id == "traffic") return EventCategory::TrafficEvent;
    if (id == "infrastructure") return EventCategory::InfrastructureEvent;
    if (id == "reliability") return EventCategory::ReliabilityEvent;
    if (id == "geographic") return EventCategory::GeographicEvent;
    if (id == "demand") return EventCategory::DemandEvent;
    if (id == "failure") return EventCategory::FailureEvent;
    if (id == "recovery") return EventCategory::RecoveryEvent;
    if (id == "educational") return EventCategory::EducationalEvent;
    return EventCategory::TrafficEvent;
}

EventMoment eventMomentFromId(const std::string& id)
{
    if (id == "planning_start" || id == "planning") return EventMoment::PlanningStart;
    return EventMoment::Simulation;
}

EventTriggerType triggerTypeFromId(const std::string& id)
{
    if (id == "metric_threshold") return EventTriggerType::MetricThreshold;
    if (id == "pressure_threshold") return EventTriggerType::PressureThreshold;
    if (id == "scenario_phase") return EventTriggerType::ScenarioPhase;
    return EventTriggerType::TimeBased;
}

EventMetric metricFromId(const std::string& id)
{
    if (id == "timeout_rate") return EventMetric::TimeoutRate;
    if (id == "retry_rate") return EventMetric::RetryRate;
    if (id == "database_queue") return EventMetric::DatabaseQueue;
    if (id == "api_queue") return EventMetric::ApiQueue;
    if (id == "cache_hit_rate") return EventMetric::CacheHitRate;
    return EventMetric::AverageLatency;
}

bool knownMetricId(const std::string& id)
{
    static const std::set<std::string> ids{
        "average_latency", "timeout_rate", "retry_rate", "database_queue", "api_queue", "cache_hit_rate",
    };
    return ids.contains(id);
}

EventEffectType effectTypeFromId(const std::string& id)
{
    if (id == "modify_traffic_rate") return EventEffectType::TrafficSpike;
    if (id == "modify_burst_intensity") return EventEffectType::TrafficSpike;
    if (id == "change_request_mix") return EventEffectType::ViralGrowth;
    if (id == "add_regional_demand" || id == "new_area_traffic") return EventEffectType::RegionalDemand;
    if (id == "degrade_node_capacity") return EventEffectType::DatabaseSlowdown;
    if (id == "unlock_action" || id == "unlock_intervention") return EventEffectType::MechanicUnlock;
    if (id == "emit_feedback") return EventEffectType::PartialRecovery;
    return EventEffectType::TrafficSpike;
}

EventLocationScope eventLocationScopeFromId(const std::string& id)
{
    if (id == "random_region") return EventLocationScope::RandomRegion;
    if (id == "region") return EventLocationScope::Region;
    if (id == "node_type") return EventLocationScope::NodeType;
    return EventLocationScope::Global;
}

ScenarioModifierType modifierTypeFromId(const std::string& id)
{
    if (id == "read_heavy_behavior") return ScenarioModifierType::ReadHeavyBehavior;
    if (id == "aggressive_retries") return ScenarioModifierType::AggressiveRetries;
    if (id == "regional_traffic_spike") return ScenarioModifierType::RegionalTrafficSpike;
    if (id == "slow_database_window") return ScenarioModifierType::SlowDatabaseWindow;
    return ScenarioModifierType::MobileRefreshWave;
}


} // namespace content::loading
