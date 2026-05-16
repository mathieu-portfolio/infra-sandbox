# Architecture Notes

The project keeps these concepts separate:

- **Node categories** describe what an infrastructure entity is, such as demand, compute, persistence, networking, or security. They live in the node registry and are used for metadata and rendering defaults.
- **Simulation layers** describe what kind of phenomenon exists in the model: Flow, Resources, Persistence, Reliability, Observability, Complexity, and Geography.
- **Runtime systems** are the code that updates simulation state each fixed timestep. Current skeletons are `RequestFlowSystem`, `QueueSystem`, `LatencySystem`, `CacheSystem`, `RetrySystem`, `FailureSystem`, and `MetricsSystem`.
- **Mechanics/interventions** are actions that transform the simulation, such as scaling up, enabling cache, clearing cache, toggling retries, or throttling traffic. Inputs should dispatch mechanic commands instead of directly mutating simulation state where practical.
- **Input actions** are normalized commands produced by `InputManager`. Controllers consume actions; they do not poll raylib directly. Intervention actions pass through `InterventionController` into `MechanicExecutor`.
- **Scenarios** define pressure progression, educational focus, objectives, allowed mechanics, traffic profiles, and phases. `ScenarioManager` applies timed phase modifiers to the simulation without owning low-level request processing.
- **Pressure analysis** interprets raw metrics and graph state into bottleneck, queue, latency, retry, and failure pressure. It produces hints/events for diagnosis UI without deciding a correct solution.
- **Overlay modes** are UI/debug views. They are independent from simulation layers and can visualize flow, latency, utilization, queues, errors, reliability, or complexity.

Topology remains part of the graph/model architecture. Scenario data remains configuration. Future economic, energy, processing, and coordination behavior should be introduced through Resources, Flow, Reliability, Persistence, or Mechanics rather than as duplicate top-level concepts.
