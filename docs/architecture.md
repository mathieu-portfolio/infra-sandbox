# Architecture Notes

The project keeps these concepts separate:

- **Node categories** describe what an infrastructure entity is, such as demand, compute, persistence, networking, or security. They live in the node registry and are used for metadata and rendering defaults.
- **Simulation layers** describe what kind of phenomenon exists in the model: Flow, Resources, Persistence, Reliability, Observability, Complexity, and Geography.
- **Runtime systems** are the code that updates simulation state each fixed timestep. Current skeletons are `RequestFlowSystem`, `QueueSystem`, `LatencySystem`, `CacheSystem`, `RetrySystem`, `FailureSystem`, and `MetricsSystem`.
- **Mechanics/interventions** are actions that transform the simulation, such as scaling up, enabling cache, clearing cache, toggling retries, or throttling traffic. Inputs should dispatch mechanic commands instead of directly mutating simulation state where practical.
- **Overlay modes** are UI/debug views. They are independent from simulation layers and can visualize flow, latency, utilization, queues, errors, reliability, or complexity.

Topology remains part of the graph/model architecture. Scenario data remains configuration. Future economic, energy, processing, and coordination behavior should be introduced through Resources, Flow, Reliability, Persistence, or Mechanics rather than as duplicate top-level concepts.
