# Sons of the Forest-Inspired Mechanics Expansion Backlog

Status: implementation specification; not a claim of completed Unreal Engine integration.
Target: Unreal Engine 5.8, C++, portable mechanics first, engine adapters second.
Safety: work on this feature branch only. Do not modify or merge into main without explicit approval.

## Goal

Raise the project's survival, construction, defense, world interaction and persistence mechanics toward a coherent systemic standard. Treat Sons of the Forest as a reference for physical assembly and survival loops, not as a requirement to copy proprietary implementation or assets. The goal is deeper interacting rules, not a thin placeholder formula.

## Required engineering gates

Every mechanic must pass all of these before it can be called complete:

1. Domain model and authoritative state are explicit.
2. Inputs are validated; invalid or stale requests fail without partial mutation.
3. State changes are transactional where multiple objects or inventories are affected.
4. Dependencies are expressed through interfaces/events, not hidden direct coupling.
5. Deterministic unit tests cover success, rejection, boundary, stale revision and rollback paths.
6. Work is bounded by LOW_MID budgets; HIGH_END may increase fidelity behind the same interfaces.
7. Save/load preserves identity, version, relationships and in-progress states.
8. Unreal adapter has UBT/UHT, Automation, PIE and teardown evidence before integration is declared done.
9. Performance is measured with representative workloads; a passing portable test is not a benchmark.
10. Every mechanic receives a source-quality rating; any score below 4/5 requires a corrective task.

## Workstream A — Shared world observation and interaction

- Keep raycast as the low-cost primary target selector.
- Add demand-driven shape traces, overlap queries, bounded surface sampling, bounds and physics snapshots through the shared scanner.
- Separate raw observations from classification and from permission to act.
- Classify trees, logs, planks, loose stones, rock, soil, landscape, constructed pieces and dynamic/falling objects.
- Attach source revisions and stable target IDs to observations. Reject stale observations at commit time.
- Cache only revision-valid results; cap hit counts, candidate counts and sample budgets.
- Acceptance: deterministic hit ordering, ignore filters, invalid-normal handling, stale target rejection, query-budget tests.

## Workstream B — Global physical assembly and placement

- Use a generic snap resolver with provider-defined snap points and groups; do not hard-code the resolver for logs.
- Log-on-log connections must have explicit compatibility, occupancy, orientation and support rules.
- Vertical logs may use a configured penetration tolerance; angled placements use allowed angular bands and connection constraints.
- Planks support configurable spacing, including a 20 cm grid, without imposing that grid on other objects.
- Placement is a staged operation: discover candidates, score, preview, revalidate collision/region/revision/support, then commit.
- A placed piece must not become a free-falling physics body merely because its visual transform was set; its support/attachment state must be explicit.
- Acceptance: deterministic ties, collision rejection, occupied snap point, support loss, rollback, rotation and spacing tests.

## Workstream C — Soil excavation and smooth terrain

- Keep authoritative terrain state in sparse density/voxel data; meshes, collision and navigation are derived products.
- Soil and rock have distinct material capabilities. Digging is permitted only where classification and material rules allow it; solid rock remains non-diggable unless a specific tool/material rule enables it.
- Use a hybrid excavation shape model (sphere/box/capsule or authored tool profile), bounded per operation.
- Build chunk snapshots with borders and source revisions; mesh workers consume immutable snapshots only.
- Publish mesh/collision on the game thread only if the source revision still matches.
- Dirty chunks are coalesced and processed under explicit per-frame budgets.
- Acceptance: adjacent-chunk seams, repeated edits, undo/rollback, stale worker result, collision publication, save/load and budget-overflow tests.

## Workstream D — Structural support, shoring and collapse

- Distinguish physical pieces, connection graph, support anchors and derived stability assessment.
- Aggregate dependent loads over the active support graph; define capacity, safety margin, damage, repair and failure states.
- Support removal/damage triggers a bounded reassessment, not a full unbounded world scan.
- Collapse is a state transition with deterministic affected-piece selection and a safe handoff to Unreal physics.
- Acceptance: shared-support overload, cycles/invalid graph rejection, damage thresholds, repair, collapse idempotency and persistence tests.

## Workstream E — Survival and environmental simulation

- Survival state includes health, stamina, hunger, thirst, temperature/exposure and condition modifiers; use explicit units and bounded rates.
- Actions consume stamina according to action type, carried load, condition and environment; no single universal cost formula.
- Environment cells own water, soil moisture, temperature, nutrients, fuel and fire state. Use explicit connectivity and mass/overflow accounting.
- Water transfer, infiltration, evaporation, drainage and nutrient leaching run on fixed steps with bounded substeps and retained time debt.
- Fire requires valid ignition conditions, consumes fuel/moisture, transfers heat to connected cells and respects wind direction.
- Agriculture uses soil suitability, moisture, nutrients, temperature, plant species stages and bounded growth updates.
- Animal care/reproduction is stateful and deterministic; avoid unbounded per-frame population scans.
- Acceptance: conservation/bounds tests, pause/large-delta tests, dry/wet edge cases, fire spread limits, plant growth stages and survival exposure integration.

## Workstream F — Raids and defensive systems

- A raid director owns wave scheduling, threat budget, target selection and cooldowns; it does not spawn actors directly from portable code.
- Target scoring considers threat, access routes, strategic value, defensive exposure and recent target history.
- Spawn preflight reports typed outcomes: approved, no candidate, budget exhausted, navigation unavailable, blocked or stale.
- Defenses have material, durability, repair, placement, coverage and interaction rules. Traps use explicit trigger volume/state/cooldown/reset and target eligibility.
- Raid pressure should respond to player/world activity and region state, with bounded escalation and recovery.
- Expensive pathfinding, large candidate sets and wave planning must be budgeted and staggered.
- Acceptance: deterministic target ties, unavailable navigation, exhausted budget, invalid target, defensive coverage, trap repeat/cooldown and large-wave stress tests.

## Workstream G — Crafting, inventory and workstations

- Crafting uses a preflight plan, source inventory revision and snapshot fingerprint. It must not consume inputs before output capacity is guaranteed.
- Validate item identity, quantity, stack size, slot constraints, weight and equipment/locked-slot rules.
- Workstations expose capability and recipe filters; crafting is not a direct UI-only mutation.
- Commit world outputs and inventory changes atomically or compensate explicitly on failure.
- Acceptance: insufficient ingredients, full inventory, output overflow, stale inventory, duplicate request and rollback tests.

## Workstream H — Unified persistence

- Use a versioned world snapshot with stable IDs and explicit schema migration.
- Save authoritative state, not generated mesh/collision caches.
- Load into staged state; validate references and cross-system invariants before replacing the live world.
- If any required participant fails, discard staged state and retain the previous live state.
- Cover logs, trees, loose stones, constructed pieces, terrain edits, shoring, survival, crops, animals, traps, raid state and inventory as each adapter becomes available.
- Moving/dragging/held/fragmented objects must have explicit persistence policies; reject unsupported transient states clearly.
- Acceptance: old schema migration, corrupt record, missing definition, duplicate ID, failed participant rollback and restart/load end-to-end tests.

## Implementation order

1. Audit current source and PR stack; identify duplicate ownership and missing real call paths.
2. Shared contracts, budgets, revisions and transactional primitives.
3. Scanner/classification and scan-authorized interaction.
4. Generic placement/snap and physical assembly.
5. Terrain density, meshing and collision publication.
6. Structural support/shoring/collapse.
7. Survival plus water/soil/fire/agriculture.
8. Raid director, defenses and traps.
9. Crafting/inventory/workstation transaction.
10. Unified persistence and migration.
11. Unreal adapter, Automation, PIE and performance gates.

## Required status reporting

For every phase, report: files added/changed; authoritative owner; real call path; tests actually executed; CI run and commit if available; known warnings; Unreal-only work remaining; source-quality score by mechanic. Never describe static test-name matching as tests passing, and never describe portable C++ tests as Unreal verification.

## Reference boundary

This is a mechanics-design and implementation backlog inspired by observable survival/building gameplay patterns. It does not copy game source code, proprietary assets or undocumented internals.
