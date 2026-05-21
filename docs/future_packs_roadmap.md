# Pack Roadmap

## Shared Foundation

### `common`
Purpose:
Reusable systems vocabulary and shared infrastructure.

### Contains
- shared actions
- shared events
- shared objectives
- topology templates
- traffic patterns
- balancing defaults
- observability systems
- UI resources

---

# Current Packs

## 1. `vanilla`
Status: Implemented

Purpose:
Introductory distributed systems concepts.

### Main Topics
- latency
- queues
- scaling
- retries
- caching
- regional traffic

### Educational Goal
Understand how pressure appears and propagates through distributed infrastructure.

---

## 2. `reliability_ops`
Status: Implemented

Purpose:
Operational reliability and incident management.

### Main Topics
- retry storms
- degradation
- observability
- stabilization
- operational recovery

### Educational Goal
Learn how systems fail under pressure and how operators stabilize them.

---

## 3. `networking_focus`
Status: Implemented

Purpose:
Networking and geographic infrastructure behavior.

### Main Topics
- latency
- routing
- congestion
- packet instability
- regional balancing

### Educational Goal
Understand how data movement shapes distributed systems.

---

## 4. `data_systems`
Status: Implemented

Purpose:
Async pipelines and large-scale data propagation.

### Main Topics
- workers
- queues
- replication
- write pressure
- analytics contention

### Educational Goal
Understand how information flows, accumulates, and destabilizes systems.

---

# Planned Packs

## 5. `frontend_focus`
Priority: High

Purpose:
Client-side systems and user experience simulation.

### Planned Mechanics
- device profiles
- rendering pressure
- asset delivery
- persistent sessions
- perceived latency

### Educational Goal
Understand how backend behavior translates into user experience.

---

## 6. `hardware_systems`
Priority: High

Purpose:
Low-level infrastructure constraints.

### Planned Mechanics
- compute saturation
- memory pressure
- storage throughput
- bandwidth limitations

### Educational Goal
Understand how physical constraints affect software architecture.

---

## 7. `security_focus`
Priority: Medium

Purpose:
Security and abuse-resistance systems.

### Planned Mechanics
- DDoS traffic
- authentication pressure
- rate limiting
- trust boundaries
- encryption overhead

### Educational Goal
Understand how adversarial traffic and protection systems reshape infrastructure.

---

## 8. `ai_infrastructure`
Priority: Medium

Purpose:
Modern AI and accelerator infrastructure.

### Planned Mechanics
- GPU clusters
- inference routing
- training pipelines
- model serving pressure
- memory-heavy workloads

### Educational Goal
Understand how AI workloads differ from classical distributed systems.

---

## 9. `streaming_media`
Priority: Medium

Purpose:
Realtime delivery and media distribution systems.

### Planned Mechanics
- live traffic bursts
- CDN distribution
- bitrate adaptation
- realtime synchronization

### Educational Goal
Understand large-scale realtime delivery constraints.

---

## 10. `mobile_ecosystems`
Priority: Low

Purpose:
Mobile-first distributed infrastructure.

### Planned Mechanics
- unstable connectivity
- battery/network constraints
- offline synchronization
- mobile latency

### Educational Goal
Understand how mobile environments reshape system design.

---

# Recommended Implementation Order

1. vanilla
2. reliability_ops
3. networking_focus
4. data_systems
5. frontend_focus
6. hardware_systems
7. security_focus
8. ai_infrastructure
9. streaming_media
10. mobile_ecosystems

---

# Recommended Core Simulation Priorities

## Highest Priority
- diagnosis gameplay
- cascading propagation
- deployment mechanics
- consistency mechanics

## Medium Priority
- frontend simulation
- hardware specialization
- observability trade-offs

## Lower Priority
- AI infrastructure
- media delivery
- advanced automation
