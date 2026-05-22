# Simulation And Content

## Simulation

The simulation is:
- turn-based
- phase-driven
- intentionally inspectable

The world only advances when:
- the player validates progression
- scripted systems explicitly progress the simulation

Animations continue while paused, but simulation does not advance.

---

## Metrics

Core metric categories:
- traffic
- pressure
- instability
- replication
- saturation

Metrics feed both:
- gameplay systems
- visualization layers

---

## Content

The project is heavily data-driven.

Main content categories:
- nodes
- actions
- events
- objectives
- scenarios

The repository contains enough content to demonstrate the systems and rendering architecture, but not commercial-scale content density.

---

## UI

The UI is organized around:
- global HUD
- side panels
- center overlays
- topology interaction

The old debug layer was intentionally removed during cleanup to keep the project presentation-focused.

![UI Screenshot](screenshots/ui.png)
