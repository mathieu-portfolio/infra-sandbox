# Content System

Gameplay metadata lives under content packs in `content/packs/`. C++ still owns simulation, evaluation, and execution logic; JSON defines static configuration. The default pack is `content/packs/vanilla/`.

## Folders

- `pack.json`: pack id, display name, description, version, and author.
- `progression/`: progression tier definitions.
- `scenarios/`: scenario definitions and references to reusable content.
- `topology/`: topology templates with nodes, links, regions, geography, and optional network identity metadata.
- `objectives/`: reusable objective and failure-limit definitions.
- `events/`: reusable event definitions.
- `modifiers/`: reusable scenario modifiers.
- `actions/node_actions.json`: Node Action metadata for mechanics and topology mutations.
- `actions/world_actions.json`: authored World Action templates used for planning drafts.
- `traffic_patterns/` or `traffic/`: reusable traffic profile definitions.
- `balancing/`: shared gameplay tuning such as pressure thresholds, interpretation text, and history windows.

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
- `engineering_capacity`
- `turn_duration`
- `sandbox_events`

Scenario action fields are the source of truth for Node Action availability. Runtime `ScenarioRun` starts with `starting_interventions`; objectives and events can unlock entries from `unlockable_interventions`.

Scenario `engineering_capacity` defines the planning budget that refills each turn:

```json
"engineering_capacity": {
  "frontend": 1,
  "backend": 2,
  "infrastructure": 1,
  "data": 1,
  "total": 4
}
```

Domain caps limit specialized work, while `total` is the shared cross-domain cap for the turn.

Scenario and phase durations define player-facing time progression. Raw simulation seconds remain internal; the visible clock/calendar and resolution summaries use the content duration:

```json
"turn_duration": {
  "value": 3,
  "unit": "months",
  "label": "3 months of platform evolution",
  "simulation_seconds": 90.0,
  "advance_calendar": true
}
```

Phases can override the turn duration with `transition_duration`. Supported units are `seconds`, `minutes`, `days`, `months`, and `years`. `simulation_seconds` controls accelerated playback length; `value`, `unit`, and `label` control gameplay presentation and calendar advancement.

Sandbox scenarios can expose manual lab injections through `sandbox_events`. These reference normal event definitions from the pack's `events/` folder; the sandbox only chooses when to inject them.

Events may include a `location` block. Omitted location defaults to global:

```json
"location": {"scope": "region", "region": "NorthAmerica"}
```

Supported scopes are:

- `global`: effect applies through the normal scenario-wide modifier path.
- `region`: effect applies only to nodes with matching geography region.
- `random_region`: resolves to one available topology region when the event activates. Optional `regions` can constrain the candidate list.
- `node_type`: effect applies only to matching node types, for example `{"scope": "node_type", "node_type": "database"}`.

Shared balancing values live in each pack's `balancing/` folder. Current tuning supports `pressure_analysis.thresholds`, `pressure_analysis.weights`, `pressure_analysis.history`, and `pressure_analysis.text`. These values configure the existing `PressureAnalysisSystem`; they do not replace pressure-analysis logic.

World Actions are authored templates. During planning, the runtime drafts a small set and may vary intensity and resulting capacity bonus deterministically from the scenario/run state. Node Action cards are not procedurally mutated.

## Procedural Numeric Ranges

Procedural content can use either a fixed number or a range object:

```json
"traffic_multiplier": 1.25
```

```json
"traffic_multiplier": {"min": 0.8, "max": 1.4}
```

Fixed values remain valid and are treated as `min == max`. Ranges are sampled once from the scenario/run seed when runtime content is instantiated, then stored on the active scenario, event, modifier, or World Action draft. They are not resampled during frame updates. Event `intensity` scales multiplier distance from `1.0`, so recovery multipliers below `1.0` remain recovery effects. `random_region` event locations also resolve deterministically from the run seed and activation time. Validation rejects any range where `min > max`.

Range-capable fields:

- events: `intensity`, `duration_seconds`, `trigger.time_seconds`, `trigger.delay_seconds`, `effect.traffic_multiplier`, `effect.burst_multiplier`, `effect.database_capacity_multiplier`, `effect.retry_delay_multiplier`, `effect.database_heavy_share`
- traffic profiles: `base_multiplier`, `growth_per_second`
- scenario modifiers: `selection_weight`, `traffic_multiplier`, `database_heavy_share`, and `burst_override.multiplier`, `burst_override.period_seconds`, `burst_override.duration_seconds`
- scenario phases: `start_time_seconds`, `duration_seconds`, `traffic_multiplier`, and nested `burst_override` numeric fields
- World Actions: `intensity_range`, `duration_seconds`, `capacity_bonus` domain values, `pressure_resistance`, `event_intensity_multiplier`, `complexity_delta`

Node Actions/cards remain fixed authored data unless a future procedural field is explicitly added.

Traffic profiles may include an `evolution` block for advanced traffic behavior:

```json
"evolution": {
  "enabled": true,
  "pressure_sensitivity": 0.18,
  "churn_sensitivity": 0.22,
  "migration_sensitivity": 0.9,
  "reroute_pressure_sensitivity": 1.6,
  "reroute_latency_sensitivity": 0.45,
  "dynamic_retry_sensitivity": 0.35,
  "burst_amplification": 0.2
}
```

These values let demand react to client experience, migrate across alternate ingress links, reroute away from pressured or slow paths, shorten retry delay under retry pressure, and amplify authored bursts.

Examples:

```json
{
  "trigger": {"type": "time", "time_seconds": {"min": 36.0, "max": 44.0}},
  "effect": {
    "type": "modify_traffic_rate",
    "traffic_multiplier": {"min": 1.25, "max": 1.45},
    "burst_multiplier": {"min": 1.15, "max": 1.35}
  },
  "duration_seconds": {"min": 15.0, "max": 22.0}
}
```

```json
{
  "capacity_bonus": {"backend": {"min": 1, "max": 2}, "total": {"min": 1, "max": 2}},
  "pressure_resistance": {"min": 0.03, "max": 0.06},
  "intensity_range": {"min": 0.85, "max": 1.2}
}
```

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

`starter -> local_startup -> database_bottleneck -> burst_traffic -> transatlantic_latency`

## Validation

The loader reports:

- duplicate IDs
- missing required IDs
- missing content folders
- unresolved scenario references
- scenarios without topology, links, or objectives
- invalid negative node or traffic numeric ranges
- invalid procedural ranges where `min` exceeds `max`
- invalid action numeric ranges such as negative complexity cost, negative region slot usage, or scale limits below 1
- invalid engineering domains, negative engineering costs, or impossible scenario capacity caps
- invalid scenario or phase durations, including non-positive gameplay values or simulation seconds

Load errors are emitted through raylib logs and displayed in the debug UI. If content loading fails, the registry installs a tiny fallback scenario so the app remains usable.

## Adding a Scenario

1. Add or reuse a topology template in the pack's `topology/`.
2. Add or reuse a traffic profile in the pack's `traffic/`.
3. Add objective and failure-limit references from the pack's `objectives/`.
4. Add scenario JSON under the pack's `scenarios/`.
5. Reference actions, events, and modifiers by ID.
6. Run the app or tests and check the debug UI for content errors.

## Adding a Node Action

Add an entry in the pack's `actions/node_actions.json` with:

- `kind`: `mechanic` or `topology_mutation`
- `mechanic`: existing C++ mechanic ID
- `mutation`: required for topology mutations
- `expected_benefits`
- `tradeoffs`
- `icon_id`
- `positive_effects`
- `negative_effects`
- `pressure_shifts`
- `node_types`: selected node types where the action appears
- `categories`: lightweight groups such as `Scaling`, `Optimization`, or `Reliability`
- `affected_pressures`: pressure categories that make the action contextually relevant
- `useful_when`: short diagnostic conditions shown in the preview
- `architectural_pattern` and `technology_example`: future discovery hooks from concrete action to pattern and real-world technology
- `complexity_cost`
- `engineering_costs`: per-turn planning cost by domain, for example `{"backend": 1, "data": 1}`
- `max_scale_level` and `diminishing_return` for scale actions
- `region_slot_usage` for topology-expanding actions
- simple `availability` metadata

The JSON controls action-card metadata, preview wording, engineering domain cost, complexity impact, scale caps, and regional slot usage. The actual mechanic or topology mutation still executes in C++.

## Adding a World Action

Add an entry in the pack's `actions/world_actions.json` with:

- `display_name`
- `description`
- `categories`
- `useful_when`
- `tradeoffs`
- `icon_id`
- `affected_pressures`
- `capacity_bonus`
- optional `pressure_resistance`, `event_intensity_multiplier`, `complexity_delta`, and `intensity_range`

World Actions are strategic planning choices. They can change the turn's engineering capacity and record broad organizational effects, but they do not directly replace Node Action execution logic.
