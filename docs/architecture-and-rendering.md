# Architecture And Rendering

## Core Structure

The project is separated into a few large domains:
- simulation
- presentation
- rendering
- content
- UI

The most important architectural decision is the separation between:
- authoritative simulation state
- continuous presentation state

Simulation is:
- discrete
- phase-based
- deterministic

Presentation is:
- smoothed
- animated
- visual only

This allows:
- smooth overlays
- alive-looking topology views
- readable transitions
- pause-friendly rendering

without advancing simulation time.

---

## Layered Views

The topology renderer is split into specialized infrastructure lenses.

Shared between all views:
- topology layout
- semantic colors
- camera/navigation

Specific to each view:
- overlay language
- rendering priorities
- visual feedback style

### Overview

Global diagnosis layer.

Keeps:
- generic warning halos
- high-level readability
- simplified infrastructure visibility

![Overview Layer](screenshots/overview.png)

### Traffic

Focuses on:
- animated flows
- congestion
- throughput

![Traffic Layer](screenshots/traffic.png)

### Resources

Focuses on:
- saturation
- gauges
- capacity usage

![Resources Layer](screenshots/resources.png)

### Reliability

Focuses on:
- instability
- dependency risk
- propagation

![Reliability Layer](screenshots/reliability.png)

### Geography

Focuses on:
- spatial grouping
- regional readability
- topology distribution

![Geography Layer](screenshots/geography.png)

---

## Project Freeze

The repository is intentionally frozen at an architectural milestone.

The current value of the project is mainly:
- rendering architecture
- layered visualization
- simulation separation
- systems-oriented UI exploration

The next logical step would mostly be large-scale content and balancing work rather than solving major technical problems.
