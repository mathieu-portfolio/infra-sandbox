# Content System

Gameplay metadata lives under `content/`. C++ still owns simulation, evaluation, and execution logic; JSON defines static configuration.

## Folders

- `progression/`: progression tier definitions.
- `scenarios/`: scenario definitions and references to reusable content.
- `topology/`: topology templates with nodes, links, regions, geography, and optional network identity metadata.
- `objectives/`: reusable objective and failure-limit definitions.
- `events/`: reusable event definitions.
- `modifiers/`: reusable scenario modifiers.
- `interventions/`: UI/action metadata for mechanics and topology mutations.
- `traffic/`: reusable traffic profile definitions.

## Definition Rules

All content definitions should include:

- `id`: stable machine identifier, unique within the definition type.
- `display_name`: user-facing name.
- `description`: readable explanation.
- `tags`: optional list for filtering and grouping.

Scenarios reference reusable definitions by ID:

- `topology_template`
- `traffic_profile`
- `objectives`
- `failure_conditions`
- `events`
- `modifiers`
- `allowed_interventions`
- `recommended_interventions`
- `starting_interventions`
- `unlockable_interventions`
- `disabled_interventions`
- `unlocks_scenarios`
- `required_completed_scenarios`

Scenario intervention fields are the source of truth for action availability. Runtime `ScenarioRun` starts with `starting_interventions`; objectives and events can unlock entries from `unlockable_interventions`.

## Objective Chains And Rewards

Scenario `objectives` may be string IDs or chain entries:

```json
{
  "objective_id": "detect_persistence_pressure",
  "starts_active": true,
  "next_objectives": ["observe_scale_limited"],
  "rewards": [
    {"type": "unlock_intervention", "id": "scale_up"},
    {"type": "emit_feedback", "message": "Scaling is available. Test the bottleneck."}
  ]
}
```

Supported reward types:

- `unlock_intervention`
- `unlock_metric`
- `unlock_overlay`
- `unlock_scenario_phase`
- `unlock_scenario`
- `emit_feedback`
- `complete_scenario`

Objective definitions may use pressure-aware conditions:

- `survive_duration`
- `pressure_detected`
- `pressure_below`
- `metric_below`
- `action_used`

Pressure-aware fields include `pressure`, `metric`, `threshold`, and optional `target_node`. Pressure analysis remains diagnostic: objectives query interpreted pressure state, but pressure analysis does not own progression.

## Scenario Roadmap

Scenario completion updates runtime `ProgressionState`:

- `completed_scenarios`
- `unlocked_scenarios`
- `unlocked_concepts`
- `unlocked_metrics`
- `unlocked_overlays`

The starter roadmap is data-driven:

`first_request -> local_startup -> database_bottleneck -> burst_traffic -> transatlantic_latency`

## Validation

The loader reports:

- duplicate IDs
- missing required IDs
- missing content folders
- unresolved scenario references
- scenarios without topology, links, or objectives
- invalid negative node or traffic numeric ranges

Load errors are emitted through raylib logs and displayed in the debug UI. If content loading fails, the registry installs a tiny fallback scenario so the app remains usable.

## Adding a Scenario

1. Add or reuse a topology template in `content/topology/`.
2. Add or reuse a traffic profile in `content/traffic/`.
3. Add objective and failure-limit references from `content/objectives/`.
4. Add scenario JSON under `content/scenarios/`.
5. Reference interventions, events, and modifiers by ID.
6. Run the app or tests and check the debug UI for content errors.

## Adding an Intervention

Add an entry in `content/interventions/` with:

- `kind`: `mechanic` or `topology_mutation`
- `mechanic`: existing C++ mechanic ID
- `mutation`: required for topology mutations
- `expected_benefits`
- `tradeoffs`
- simple `availability` metadata

The JSON controls action-card metadata. The actual mechanic or topology mutation still executes in C++.
