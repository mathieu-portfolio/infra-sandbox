#pragma once

#include <cstdint>
#include <span>
#include <string_view>

enum class NodeCategory {
    Demand,
    Compute,
    Persistence,
    Acceleration,
    Coordination,
    Networking,
    Reliability,
    Observability,
    Infrastructure,
    Security
};

enum class NodeType {
    ClientCluster,
    ExternalService,
    BotSource,
    ApiService,
    Microservice,
    Worker,
    BatchProcessor,
    StreamProcessor,
    AIInferenceNode,
    AITrainingCluster,
    Database,
    ReadReplica,
    Shard,
    ObjectStorage,
    VectorDatabase,
    SearchIndex,
    Cache,
    CDNEdge,
    QueryAccelerator,
    CompressionNode,
    QueueBroker,
    EventBus,
    Orchestrator,
    ServiceRegistry,
    ConsensusNode,
    LoadBalancer,
    Gateway,
    Proxy,
    EdgeRouter,
    Firewall,
    CircuitBreaker,
    FailoverController,
    HealthMonitor,
    RateLimiter,
    RetryController,
    MetricsCollector,
    LoggingNode,
    TraceCollector,
    AlertManager,
    AnalyticsDashboard,
    ComputeCluster,
    GPUCluster,
    StorageCluster,
    Datacenter,
    Region,
    EdgeZone,
    AuthService,
    WAF,
    SecretVault,
    EncryptionService
};

enum class NodeRenderStyle {
    DemandPulse,
    ComputeBox,
    PersistenceBlock,
    AccelerationCache,
    CoordinationDiamond,
    NetworkingHub,
    ReliabilityShield,
    ObservabilityLens,
    InfrastructureCluster,
    SecurityLock,
    Generic
};

struct NodeVisualColor {
    std::uint8_t r = 120;
    std::uint8_t g = 140;
    std::uint8_t b = 160;
    std::uint8_t a = 255;
};

struct NodeDefinition {
    NodeType type = NodeType::ApiService;
    std::string_view displayName;
    NodeCategory category = NodeCategory::Compute;
    NodeRenderStyle renderStyle = NodeRenderStyle::Generic;
    bool generatesRequests = false;
    bool processesRequests = false;
    bool storesState = false;
    bool buffersRequests = false;
    bool routesRequests = false;
    double defaultProcessingCapacityPerSecond = 0.0;
    double defaultRequestRatePerSecond = 0.0;
    float defaultVisualSize = 72.0f;
    NodeVisualColor color{};
};

class NodeRegistry {
public:
    [[nodiscard]] static const NodeDefinition& definition(NodeType type);
    [[nodiscard]] static std::span<const NodeDefinition> definitions();
    [[nodiscard]] static NodeCategory categoryOf(NodeType type);
    [[nodiscard]] static NodeRenderStyle renderStyleOf(NodeType type);
    [[nodiscard]] static bool generatesRequests(NodeType type);
    [[nodiscard]] static bool processesRequests(NodeType type);
    [[nodiscard]] static bool storesState(NodeType type);
    [[nodiscard]] static bool buffersRequests(NodeType type);
    [[nodiscard]] static bool routesRequests(NodeType type);
};
