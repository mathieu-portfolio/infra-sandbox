#include "core/topology/NodeDefinition.hpp"

#include <array>
#include <cassert>

namespace {
constexpr NodeVisualColor demandColor{88, 166, 255, 255};
constexpr NodeVisualColor computeColor{86, 210, 151, 255};
constexpr NodeVisualColor persistenceColor{245, 184, 76, 255};
constexpr NodeVisualColor accelerationColor{78, 201, 176, 255};
constexpr NodeVisualColor coordinationColor{187, 128, 255, 255};
constexpr NodeVisualColor networkingColor{121, 192, 255, 255};
constexpr NodeVisualColor reliabilityColor{235, 86, 100, 255};
constexpr NodeVisualColor observabilityColor{139, 148, 158, 220};
constexpr NodeVisualColor infrastructureColor{176, 196, 222, 255};
constexpr NodeVisualColor securityColor{255, 214, 102, 255};

constexpr NodeRenderStyle styleFor(NodeCategory category)
{
    switch (category) {
    case NodeCategory::Demand:
        return NodeRenderStyle::DemandPulse;
    case NodeCategory::Compute:
        return NodeRenderStyle::ComputeBox;
    case NodeCategory::Persistence:
        return NodeRenderStyle::PersistenceBlock;
    case NodeCategory::Acceleration:
        return NodeRenderStyle::AccelerationCache;
    case NodeCategory::Coordination:
        return NodeRenderStyle::CoordinationDiamond;
    case NodeCategory::Networking:
        return NodeRenderStyle::NetworkingHub;
    case NodeCategory::Reliability:
        return NodeRenderStyle::ReliabilityShield;
    case NodeCategory::Observability:
        return NodeRenderStyle::ObservabilityLens;
    case NodeCategory::Infrastructure:
        return NodeRenderStyle::InfrastructureCluster;
    case NodeCategory::Security:
        return NodeRenderStyle::SecurityLock;
    }
    return NodeRenderStyle::Generic;
}

constexpr NodeVisualColor colorFor(NodeCategory category)
{
    switch (category) {
    case NodeCategory::Demand:
        return demandColor;
    case NodeCategory::Compute:
        return computeColor;
    case NodeCategory::Persistence:
        return persistenceColor;
    case NodeCategory::Acceleration:
        return accelerationColor;
    case NodeCategory::Coordination:
        return coordinationColor;
    case NodeCategory::Networking:
        return networkingColor;
    case NodeCategory::Reliability:
        return reliabilityColor;
    case NodeCategory::Observability:
        return observabilityColor;
    case NodeCategory::Infrastructure:
        return infrastructureColor;
    case NodeCategory::Security:
        return securityColor;
    }
    return {};
}

constexpr NodeDefinition node(
    NodeType type,
    std::string_view displayName,
    NodeCategory category,
    bool generatesRequests = false,
    bool processesRequests = false,
    bool storesState = false,
    bool buffersRequests = false,
    bool routesRequests = false,
    double capacity = 0.0,
    double requestRate = 0.0,
    float visualSize = 72.0f)
{
    return {
        .type = type,
        .displayName = displayName,
        .category = category,
        .renderStyle = styleFor(category),
        .generatesRequests = generatesRequests,
        .processesRequests = processesRequests,
        .storesState = storesState,
        .buffersRequests = buffersRequests,
        .routesRequests = routesRequests,
        .defaultProcessingCapacityPerSecond = capacity,
        .defaultRequestRatePerSecond = requestRate,
        .defaultVisualSize = visualSize,
        .color = colorFor(category),
    };
}

constexpr std::array<NodeDefinition, 50> kDefinitions = {
    node(NodeType::ClientCluster, "Client Cluster", NodeCategory::Demand, true, false, false, false, false, 0.0, 4.0, 72.0f),
    node(NodeType::ExternalService, "External Service", NodeCategory::Demand, true, true, false, false, true),
    node(NodeType::BotSource, "Bot Source", NodeCategory::Demand, true, false, false, false, false, 0.0, 8.0),
    node(NodeType::ApiService, "API Service", NodeCategory::Compute, false, true, false, true, true, 7.0, 0.0, 96.0f),
    node(NodeType::Microservice, "Microservice", NodeCategory::Compute, false, true, false, true, true, 5.0),
    node(NodeType::Worker, "Worker", NodeCategory::Compute, false, true, false, true, false, 4.0),
    node(NodeType::BatchProcessor, "Batch Processor", NodeCategory::Compute, false, true, false, true, false, 2.0),
    node(NodeType::StreamProcessor, "Stream Processor", NodeCategory::Compute, false, true, false, true, true, 5.0),
    node(NodeType::AIInferenceNode, "AI Inference Node", NodeCategory::Compute, false, true, false, true, false, 2.0),
    node(NodeType::AITrainingCluster, "AI Training Cluster", NodeCategory::Compute, false, true, true, true, false, 1.0, 0.0, 110.0f),
    node(NodeType::Database, "Database", NodeCategory::Persistence, false, true, true, true, false, 4.0, 0.0, 84.0f),
    node(NodeType::ReadReplica, "Read Replica", NodeCategory::Persistence, false, true, true, true, false, 8.0),
    node(NodeType::Shard, "Shard", NodeCategory::Persistence, false, true, true, true, false, 4.0),
    node(NodeType::ObjectStorage, "Object Storage", NodeCategory::Persistence, false, true, true, true, false, 6.0),
    node(NodeType::VectorDatabase, "Vector Database", NodeCategory::Persistence, false, true, true, true, false, 3.0),
    node(NodeType::SearchIndex, "Search Index", NodeCategory::Persistence, false, true, true, true, false, 5.0),
    node(NodeType::Cache, "Cache", NodeCategory::Acceleration, false, true, true, true, false, 12.0, 0.0, 76.0f),
    node(NodeType::CDNEdge, "CDN Edge", NodeCategory::Acceleration, false, true, true, true, true, 15.0),
    node(NodeType::QueryAccelerator, "Query Accelerator", NodeCategory::Acceleration, false, true, true, true, false, 8.0),
    node(NodeType::CompressionNode, "Compression Node", NodeCategory::Acceleration, false, true, false, true, false, 6.0),
    node(NodeType::QueueBroker, "Queue Broker", NodeCategory::Coordination, false, true, true, true, true, 10.0),
    node(NodeType::EventBus, "Event Bus", NodeCategory::Coordination, false, true, true, true, true, 12.0),
    node(NodeType::Orchestrator, "Orchestrator", NodeCategory::Coordination, false, true, true, true, true, 4.0),
    node(NodeType::ServiceRegistry, "Service Registry", NodeCategory::Coordination, false, true, true, false, true, 8.0),
    node(NodeType::ConsensusNode, "Consensus Node", NodeCategory::Coordination, false, true, true, true, true, 2.0),
    node(NodeType::LoadBalancer, "Load Balancer", NodeCategory::Networking, false, true, false, true, true, 20.0),
    node(NodeType::Gateway, "Gateway", NodeCategory::Networking, false, true, false, true, true, 15.0),
    node(NodeType::Proxy, "Proxy", NodeCategory::Networking, false, true, false, true, true, 15.0),
    node(NodeType::EdgeRouter, "Edge Router", NodeCategory::Networking, false, true, false, true, true, 20.0),
    node(NodeType::Firewall, "Firewall", NodeCategory::Networking, false, true, false, true, true, 18.0),
    node(NodeType::CircuitBreaker, "Circuit Breaker", NodeCategory::Reliability, false, false, false, true, true),
    node(NodeType::FailoverController, "Failover Controller", NodeCategory::Reliability, false, true, true, false, true, 4.0),
    node(NodeType::HealthMonitor, "Health Monitor", NodeCategory::Reliability, false, true, true, false, false, 5.0),
    node(NodeType::RateLimiter, "Rate Limiter", NodeCategory::Reliability, false, true, true, true, true, 12.0),
    node(NodeType::RetryController, "Retry Controller", NodeCategory::Reliability, false, true, true, true, true, 8.0),
    node(NodeType::MetricsCollector, "Metrics Collector", NodeCategory::Observability, false, true, true, true, false, 8.0),
    node(NodeType::LoggingNode, "Logging Node", NodeCategory::Observability, false, true, true, true, false, 8.0),
    node(NodeType::TraceCollector, "Trace Collector", NodeCategory::Observability, false, true, true, true, false, 6.0),
    node(NodeType::AlertManager, "Alert Manager", NodeCategory::Observability, false, true, true, true, true, 4.0),
    node(NodeType::AnalyticsDashboard, "Analytics Dashboard", NodeCategory::Observability, false, true, true, false, false, 4.0),
    node(NodeType::ComputeCluster, "Compute Cluster", NodeCategory::Infrastructure, false, true, false, true, true, 20.0, 0.0, 110.0f),
    node(NodeType::GPUCluster, "GPU Cluster", NodeCategory::Infrastructure, false, true, false, true, false, 8.0, 0.0, 110.0f),
    node(NodeType::StorageCluster, "Storage Cluster", NodeCategory::Infrastructure, false, true, true, true, false, 12.0, 0.0, 110.0f),
    node(NodeType::Datacenter, "Datacenter", NodeCategory::Infrastructure, false, true, true, true, true, 50.0, 0.0, 130.0f),
    node(NodeType::Region, "Region", NodeCategory::Infrastructure, false, true, true, true, true, 80.0, 0.0, 150.0f),
    node(NodeType::EdgeZone, "Edge Zone", NodeCategory::Infrastructure, false, true, true, true, true, 30.0, 0.0, 120.0f),
    node(NodeType::AuthService, "Auth Service", NodeCategory::Security, false, true, true, true, false, 8.0),
    node(NodeType::WAF, "WAF", NodeCategory::Security, false, true, true, true, true, 16.0),
    node(NodeType::SecretVault, "Secret Vault", NodeCategory::Security, false, true, true, true, false, 5.0),
    node(NodeType::EncryptionService, "Encryption Service", NodeCategory::Security, false, true, false, true, false, 8.0),
};

const NodeDefinition& fallbackDefinition()
{
    static constexpr NodeDefinition fallback = node(NodeType::ApiService, "Unknown Node", NodeCategory::Infrastructure);
    return fallback;
}
}

const NodeDefinition& NodeRegistry::definition(NodeType type)
{
    const auto index = static_cast<std::size_t>(type);
    if (index >= kDefinitions.size()) {
        return fallbackDefinition();
    }

    const auto& definition = kDefinitions[index];
    assert(definition.type == type);
    return definition.type == type ? definition : fallbackDefinition();
}

std::span<const NodeDefinition> NodeRegistry::definitions()
{
    return kDefinitions;
}

NodeCategory NodeRegistry::categoryOf(NodeType type)
{
    return definition(type).category;
}

NodeRenderStyle NodeRegistry::renderStyleOf(NodeType type)
{
    return definition(type).renderStyle;
}

bool NodeRegistry::generatesRequests(NodeType type)
{
    return definition(type).generatesRequests;
}

bool NodeRegistry::processesRequests(NodeType type)
{
    return definition(type).processesRequests;
}

bool NodeRegistry::storesState(NodeType type)
{
    return definition(type).storesState;
}

bool NodeRegistry::buffersRequests(NodeType type)
{
    return definition(type).buffersRequests;
}

bool NodeRegistry::routesRequests(NodeType type)
{
    return definition(type).routesRequests;
}
