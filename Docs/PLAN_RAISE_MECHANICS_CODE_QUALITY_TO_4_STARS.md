# Plan — Raise Every Below-4 Mechanic to at Least 4 Stars

**Date:** 23 October 2026  
**Target branch:** modules/mechanics-pre-unreal  
**Scope:** mechanics below four stars in Docs/MECHANIC_CODE_QUALITY_RATING_01.md, covering main-branch systems and portable module prototypes.

## 1. What four stars means

The goal is minimum ★★★★☆ source-code quality for every currently sub-4 mechanic. This rating concerns algorithm depth, state/data integrity, responsibility boundaries, complexity, error handling and maintainability. It is separate from build/test/PIE verification.

A mechanic reaches four stars when it has:
1. A complete gameplay-domain model, not just a wrapper or a few formulas.
2. Explicit invariants for IDs, units, revisions, bounds, ownership and state transitions.
3. Safe state mutation: preflight, stage, commit and rollback/compensation.
4. Cost proportional to affected regions/candidates where practical, with explicit work/memory budgets.
5. Clear separation of policy, algorithm, engine adapter, orchestration and presentation.
6. Deterministic decisions when gameplay requires it.
7. Readable functions, named configuration instead of unexplained constants, and no silent success on required-step failure.
8. Distinct failure outcomes for invalid input, stale data, unsupported providers, budget overflow and failed commits.

Four stars does not mean physically exact engineering or full fluid dynamics. The target is a deep, deterministic, well-bounded game simulation whose approximations are explicit.

## 2. Order of execution

Do not rewrite every system simultaneously. The dependency order is:

1. Shared conventions and state-transaction patterns.
2. Source-loss/duplication bugs in wood cutting, inventory and persistence.
3. Water flow and structural-load algorithms.
4. WorldScan → classification → excavation.
5. Voxel meshing/collision → placement/snap.
6. Shoring and structural integration.
7. Fire, environment and survival.
8. Raid targeting/spawning and chunk scheduling.
9. Plant, animal, material and crafting models.
10. Cross-cutting readability and complexity pass.

Every phase should land in a small reviewed commit/PR. The open phase branches and PR #19 must be intentionally integrated; code on a work branch is not automatically present in main.

## 3. Phase 0 — shared conventions and primitives

**Targets:** Modules/CoreContracts, Modules/RuntimeProfiles, Source/MyProject2/WorldCore/WorldTypes.h, Scripts/run_tests.sh.

Actions:
- Standardize readable C++ formatting. Expand one-line methods when they perform validation, state changes or multiple side effects.
- Define distinct IDs for world, entity, operation, plan, region, snapshot and base-density source. Avoid interchangeable integers for IDs and revisions.
- Make units explicit at system boundaries. Use meters in portable world geometry and convert Unreal centimeters in one adapter boundary.
- Establish a small, typed preflight/stage/commit/result pattern. Avoid a giant universal transaction framework; use domain-specific transactions.
- Split immutable world-grid identity (origin, density spacing, cell/chunk dimensions and base source) from tunable execution budgets (jobs, queue depth, publishes per frame).
- Ensure RuntimeProfiles locking compares every field that affects grid compatibility. It should not confuse a performance-budget adjustment with a change to the world coordinate grid.
- Make every hot loop declare an input/work limit or inherit a documented limit from its contract.
- Remove misleading-indentation and similar warnings; harden the test runner so compiler/test exit codes cannot be masked by grep or piping.

**Four-star exit rule:** the authoritative state, unit, revision, error outcome and work budget of each modified operation are identifiable without relying on undocumented side effects.

## 4. Phase 1 — transaction safety first

### 1A. Wood cutting / piece generation

Files:
- Source/MyProject2/Wood/WoodCutterComponent.cpp
- Source/MyProject2/Wood/WoodCutLibrary.cpp
- Source/MyProject2/Wood/WoodPieceComponent.cpp

Problem found: UWoodCutterComponent::Finish spawns outputs one by one. If a spawn or component lookup fails, it can continue, shrink the source piece anyway and still report success.

Implementation:
1. Split Finish into PreflightCut, StageCutOutputs, CommitCut and RollbackCut.
2. Validate all slice ranges, output count/data, world/owner, source state and source revision before spawning.
3. Create output actors in a staged list; configure data/transforms with physics and external registration disabled until every output and required component is valid.
4. On any failure, destroy every staged actor and preserve the original piece, data, transform and physics state.
5. Commit the original-piece change only after all output actors are ready. Publish one success event only after commit.
6. Treat an actor class with missing required components as an explicit error and clean it up.
7. Add an operation ID/source revision check to reject duplicate or stale cut requests.

**Four-star bar:** a split is all-or-nothing. No output can silently be lost, duplicated or half-created.

### 1B. Inventory restore, drop and crafting

Files:
- Source/MyProject2/InventoryCore/InventoryComponent.cpp
- Source/MyProject2/InventoryCore/InventorySaveSubsystem.cpp
- Source/MyProject2/InventoryWorld/
- inventory/crafting bridge code where the world transfer is orchestrated.

Problems found: RestoreFromSavedItems clears live state before completely validating/packing saved items. DropItem removes inventory counts; that method alone does not prove a world-drop actor was created successfully.

Implementation:
1. Build a staged slot array from saved records. Validate IDs, stack/count constraints, weight, locked slots, duplicate/unknown definitions and numeric bounds without changing the live inventory.
2. Define whether missing definitions can be restored from serialized data or must fail; do not invent definitions silently.
3. Stage quick-access slots against the staged inventory.
4. Swap into live inventory in one commit. On failure, preserve old inventory and quick-access state.
5. For a drop: reserve count → spawn and configure world item → finalize inventory debit. Failure destroys the staged actor and releases the reservation.
6. For crafting: preflight ingredients and output capacity, reserve inputs, stage outputs, then commit inputs/output together. Use an idempotent operation ID where retries are possible.
7. Emit delegates/UI events after commit only.

**Four-star bar:** restore, drop and crafting can fail without losing or duplicating items.

### 1C. World persistence

Files:
- Source/MyProject2/WorldPhaseW6_Persistence/WorldPersistence.cpp
- Source/MyProject2/WorldPhaseW6_Persistence/ChunkSerializer.cpp
- Source/MyProject2/WorldCore/ChunkStateStore.*
- Modules/Persistence/

Existing code already has format versioning, magic, size caps, sorted cell keys, compression and parse-before-replace. The gap is protecting the live state during the replacement itself and binding sparse terrain deltas to their proper baseline.

Implementation:
1. Deserialize into a staged ChunkStateStore. Build every chunk and validate all cross-references before mutating the live store.
2. Commit through a controlled store swap. If that is not possible, retain a rollback snapshot and restore it on any insertion failure.
3. Write saves to a temporary sibling file, flush/close and atomically replace the destination. Failed write/rename preserves the prior save.
4. Separate serialization from live-state mutation; parsing a file must never clear the current world.
5. Centralize version migration by source version.
6. Add world/map ID, base-density provider ID/version, grid origin/spacing/dimensions, grid/profile fingerprint and schema to density snapshot metadata. Reject deltas for a different base source.
7. Define explicit persistence participants for terrain, materials, water/soil, fire/decay/chemistry, construction, inventory and environment. Report missing participants instead of silently omitting them.

**Four-star bar:** invalid or partly incompatible data never leaves the live world half-replaced.

## 5. Phase 2 — WorldScan, classification and excavation as real domain algorithms

Targets:
- Modules/WorldScan/
- Modules/WorldClassification/
- Modules/Excavation/
- UE phase code under Source/MyProject2/WorldCore/ on its existing phase branch, to integrate deliberately.

### WorldScan
- Make ray, shape sweep, overlap, surface sample and required bounds/physics observations share one query-policy contract.
- Validate finite vectors, normalized direction, valid shape, range, profile, result cap, ignored actors/components and query channel consistently.
- Return raw evidence: stable target/component ID, point, normal, distance, source revision and query outcome. Do not decide whether a target is diggable in the raycast.
- Distinguish miss, unsupported provider, stale observation, invalid provider response and budget rejection.
- Implement the UE provider behind the existing portable interface; keep Unreal classes out of the portable algorithm.
- Sort multi-result queries deterministically.

### WorldClassification
- Use explicit precedence: authored override → trusted actor/component marker → Landscape layer/physical material → sourced cached evidence → low-confidence geometric inference → Unknown.
- Return class, provenance, source revision, confidence/reason and allowed capabilities.
- Replace full cells_ map copies on local update with a transaction over touched keys.
- Split per-batch count, total cell count and memory-byte budgets.
- Invalidate only the affected region/revision when possible.
- Unknown and StaticGeometry fail closed for digging unless an explicit gameplay policy says otherwise.

### Excavation and TerrainVoxel
- Define DigIntent with operation ID, target ID, tool capability, shape, range, source surface revision, classification revision and expected density revision.
- Recheck those revisions immediately before commit. Reject RawHit.sourceRevision if stale.
- Split editing into Preflight → BuildChangeSet → Commit → EmitDirtyRegion.
- A no-op edit does not increment authoritative revision; duplicate operation IDs are rejected.
- Keep rollback receipts but do not allow an old rollback to undo newer work.
- Return structured failure reasons and changed cell/chunk counts.

**Four-star bar:** only a fresh, capability-authorized, budget-valid intent can change authoritative density; failure to classify never means “editable.”

## 6. Phase 3 — density, smooth mesh, collision and placement

Targets:
- Modules/TerrainVoxel/
- Modules/MeshCollision/
- Modules/PlacementSnap/
- Source/MyProject2/WorldCore/WorldVoxelMesher.cpp and ExcavationVolumeSubsystem.cpp on the existing phase branch.

### Voxel and mesher
- Treat density/material state as authoritative; the mesh is rebuildable derived data.
- Bound sample count, estimated memory, triangle output and task size before allocation.
- Replace expensive per-edge tree-map lookups with a compact edge-key/hash or indexed cache after profiling representative worst cases.
- Cache gradients per sample/stencil instead of repeatedly sampling the same neighbors.
- Specify deterministic material selection where multiple material classes meet; match portable and UE mesher policy.
- Define exact iso-level equality and remove/reject degenerate triangles deterministically.
- Build from immutable bordered chunk snapshots. Shared border samples must match between adjacent chunks.
- Coalesce dirty chunks and bound queues by item count and estimated bytes.
- Track density, render and collision revisions independently. Stale results cannot publish as current; failures create explicit rebuild debt.
- Budget render publication, collision cooking and navigation updates separately. One dig must not force all expensive work in the same frame.

### PlacementSnap and the 20 cm plank rule
- Keep candidate ranking deterministic; create separate providers for free surface, log/socket joint, shoring joint and plank grid.
- Use the same transform and compatibility policy for preview and commit.
- Check the full oriented piece volume with actual overlap queries, not only the pivot or a prefilled overlap boolean.
- Keep source target and surface/classification revisions on the preview; reject if they changed.
- Make the 0.20 m plank grid configurable and plank-specific; do not apply it to log sockets or terrain tools.
- Commit resource reservation, actor spawn, link registration and resource debit transactionally; compensate every partial failure.
- Validate normal/axis dot thresholds and make tie-breaking explicit.

**Four-star bar:** no stale render/collision product or placement preview can become authoritative after the underlying terrain changes.

## 7. Phase 4 — StructuralIntegrity, shoring and existing wood structure analysis

Targets:
- Modules/StructuralIntegrity/
- Modules/Shoring/
- Source/MyProject2/Wood/WoodStructureLogic.cpp
- Source/MyProject2/Wood/WoodStructureAnalysis.cpp
- Source/MyProject2/Wood/WoodStructureSubsystem.cpp

### StructuralIntegrity
- Keep validated graph nodes and edges with explicit direction, supporter capacity, dependent demand, health/damage, joint type and grounded support.
- Use a topological pass for directed dependencies; reject cycles or define a documented cycle policy.
- Aggregate load reaching each supporter. Do not test each dependent independently against the supporter's full capacity.
- Define deterministic load distribution among multiple supports (equal share or a declared geometry/stiffness weighting). Clearly state that this is a game model, not a full structural engineering solver.
- Recompute only the affected connected region where safe; maintain full-graph recomputation as a debug/reference path.
- Separate Unsupported, Overloaded, Damaged, Collapsed and NeedsRevalidation states.
- Return a cause chain for collapse candidates: failed edge, missing grounded path, overload or damage threshold.

### Shoring
- Upgrade geometry proposals to use actual terrain sample points/normals, anchor depth and valid anchor ranges.
- Calculate coverage: endpoints, board row overlap/gaps, upright spacing, strut connections and side completeness.
- Store explicit assembly edges between boards, uprights and struts; a list of positions alone is not a connected shoring assembly.
- Validate anchor and collision volumes before installation. Stage all actors/links, then commit them as one assembly.
- Invalidate a plan only when an edit intersects its anchor/coverage region. A distant terrain change must not invalidate all plans.
- Make replacement/carry/remove/resource debit transactional.
- Connect Shoring to StructuralIntegrity through a typed adapter and shared load contract.

### Existing wood structure analysis
- Preserve domain rules for notches, piece roles, support paths, openings, platforms, ramps and bridges.
- Add a spatial broad phase/spatial hash to avoid all-pairs comparisons for local changes in a large construction.
- Rebuild links/features in the affected neighborhood; reuse unaffected connections.
- Move contact/orientation/feature thresholds to a typed construction-rules configuration with units and valid ranges.
- Return feature evidence so construction and UI can explain why a wall/platform/bridge was recognized or rejected.

**Four-star bar:** capacity is shared correctly, local edits stay local where possible, and every collapse/placement decision has a traceable reason.

## 8. Phase 5 — water, soil, environmental state, fire and survival

Targets:
- Source/MyProject2/WorldPhaseW4_WaterSoil/
- Source/MyProject2/BurnSubsystem.cpp
- Source/MyProject2/Campfire.cpp
- Source/MyProject2/WorldCore/WorldStateSubsystem.cpp
- Modules/WorldEnvironment/
- Modules/Survival/

### Water flow
- Define units: cell area, volume, terrain elevation, water depth and hydraulic head are separate quantities.
- Process each adjacent cell pair once per substep from an immutable snapshot; accumulate all outgoing/incoming deltas before applying them.
- Accumulate signed flow rates per cell rather than overwriting the previous neighbor's rate.
- Compute flow from terrain elevation + water depth (head), conductance/roughness and edge connectivity.
- Use bounded substeps/stability limits for large delta times.
- Preserve nonnegative volume and define numerical mass-balance tolerance. Model rainfall, infiltration, evaporation, drains and sources as explicit in/out terms.
- State clearly that the current solver is 2D unless/until a 3D solver is implemented.
- Process dirty cells/neighborhoods rather than all active cells when a local update is enough.

### Soil, environment and decay
- Define one authoritative clock and explicit units for water/soil/fire/decay/chemistry.
- Move daily constants to per-species/material/biome configuration with units.
- State the update order: water → soil water/nutrients → plant uptake; heat/fire → material damage; time/temp/moisture → decay; material properties → reaction outputs.
- Keep environmental samples source-tagged and revisioned. Missing and zero must not mean the same thing; stale samples need an explicit fallback.
- Return dirty-cell information for persistence and dependent systems.

### Fire and heat propagation
- Give active ignition/heat search one owner; remove duplicate full-world log scans from BurnSubsystem and Campfire.
- Query only objects within the heat radius through a spatial index or bounded overlap query; cache candidate pairs until movement invalidates them.
- Separate heat/temperature from ignition progress, fuel consumption and structural damage with explicit units.
- Cap work per step and per tick. Carry simulation debt forward instead of processing arbitrarily many substeps in a single frame.
- Combine heat from multiple sources under a declared rule; avoid double damage from two owners in the same step.

### Survival exposure
- Process DeltaSeconds above maxStepSeconds via bounded substeps or an accumulator; never discard elapsed time.
- Define wetness, thermal stress, darkness and water exposure accumulation/recovery/saturation in one named data-driven policy.
- Use SurvivalExposure as a calculation layer behind one authoritative character state; avoid competing sources of truth with SurvivalComponent.
- Define max age and fallbacks for missing/stale environment channels.
- Publish death/UI events only after state updates commit.

**Four-star bar:** water has consistent quantity accounting; fire scales with nearby objects; environmental channels have one owner; survival time is not silently lost.

## 9. Phase 6 — Raid, world scheduling, materials, inventory and crafting

### Raid director, target scoring and spawn finding

Files:
- Source/MyProject2/RaidCore/RaidDirectorSubsystem.cpp
- Source/MyProject2/RaidPhaseR6_Targeting/TargetScorer.cpp
- Source/MyProject2/RaidPhaseR5_Spawn/SpawnPositionFinder.cpp
- relevant planner, budget and wave-state code.

Changes:
1. Split director orchestration into readable steps: capture context, evaluate difficulty, select plan, advance wave, request budget, select target, select spawn point, spawn/track enemy, finish/cancel.
2. Capture current player status; do not hardcode every player's level to 1 or assume a pawn means alive.
3. Compute threat and accessibility from actual target-specific features: priority/type, damage/threat, path distance/cost, visibility, access and current group intent. A constant added to every target is not a meaningful scoring term.
4. Normalize score features to known comparable ranges and return a score breakdown for tuning.
5. Select the intended target per enemy/group, not simply the first alive player in the snapshot.
6. Inject deterministic RNG into spawn sampling. Avoid global FRandRange if deterministic repeat/replay is required.
7. Replace zero-vector spawn failure sentinels with a typed result/optional position.
8. Make no-navigation-mesh behavior a configured policy, not silent proof of accessibility.
9. Bound spawn attempts and ensure failed attempts consume a retry/work budget.
10. Make group composition and spawn pressure intentional and explainable, rather than relying only on weighted enemy selection.

### World chunk scheduler

- Maintain an ordered active-chunk collection instead of rebuilding and sorting every chunk on every tick.
- Separate reads from mutation: GetCellAtWorld should not mark dirty merely because a caller reads; use explicit mutation APIs.
- Ensure a rotating cursor does not starve chunks under sustained budget pressure.
- Budget estimated work/time as well as chunk count when one chunk's simulation cost can be much higher than another.
- Make lazy catch-up/weather deterministic and avoid applying rainfall twice via overlapping catch-up paths.
- Do not mark a chunk dirty when no authoritative state changed.
- Expose backlog and catch-up limits in RuntimeProfiles.

### Material M1–M6

- Validate finite/legal material properties at registry registration.
- Resolve interaction rules by explicit priority, class, specificity and type; don't depend on asset/map iteration order.
- Cache lookups by material-definition revision and invalidate on registry/matrix updates.
- Use the same immutable context and selected rule for prediction and application.
- Define the boundary between proposed reaction and authoritative material state mutation. Return before/after values, event ID, source and dirty/persistence notification.
- Keep layer ownership clear: definitions → rules → per-instance state → composites/specialized materials → world/persistence bridge.

### Inventory/crafting algorithm depth

- Cache item definitions by stable ID and reject duplicate IDs on registration.
- Use a shared stack-packing planner for CanFit, Add and Restore so capacity estimates cannot diverge from insertion.
- Validate weight/count/stack numeric ranges and malformed saved data.
- Treat recipe inputs/outputs as a reservation transaction.
- Distinguish drop, consume, transfer and remove; do not imply world-drop success from an inventory decrement.
- Publish delegates after commit only.

**Four-star bar:** raid decisions use real target features; chunk work is bounded and fair; materials have deterministic rule precedence; item mutations are consistent and retry-safe.

## 10. Phase 7 — deepen plant and animal models

Targets:
- Source/MyProject2/PhaseT2_Plant/PlantGrowthComponent.cpp
- Source/MyProject2/PhaseT2_Plant/CropFieldActor.cpp
- Source/MyProject2/PhaseT6_Animal/AnimalActor.cpp
- Source/MyProject2/PhaseT7_Production/
- Source/MyProject2/PhaseT8_Reproduction/

### Plants and soil
- Move daily constants (water use, uptake, stress response, temperature thresholds and GDD thresholds) into per-species definitions with units.
- Separate phenology, biomass, water balance, nutrient balance, plant health/stress and yield. A single suitability multiplier should not stand in for all physical/biological processes.
- Keep CropFieldActor as integration/orchestration and make growth calculation a pure model receiving explicit soil/environment inputs.
- Apply water/nutrient uptake as explicit soil fluxes; plant uptake should reduce the soil pools.
- Define consistent maturity/harvestability/senescence transitions.
- Handle skipped days with deterministic daily integration or bounded analytical catch-up.
- Ensure identical initial state/weather/seed produces identical evolution.
- Keep visuals downstream of authoritative growth stage/state.

### Animals, feeding, production and reproduction
- Split AnimalActor::UpdateDaily into clear steps: intake capacity, feed choice, feed reservation, nutrition, energy, growth, health, reproduction/pregnancy, production and state commit.
- Define a feed-selection policy (species preference, quality, available quantity and priority). Do not let unordered map iteration choose feed.
- Commit feed stock and animal state together; on failure neither changes.
- Make conception probability/time semantics explicit. Avoid accumulating a probability to a threshold then rolling again with an unexplained probability.
- Give birth, pregnancy, lactation and product output one lifecycle owner; one day event must not duplicate birth/output.
- Define product storage as amount/capacity and handle overflow explicitly.
- Inject deterministic RNG and persist seed/sequence where required.
- Remove day subscriptions on EndPlay and make daily operations idempotent by day number.

**Four-star bar:** plant resource pools balance; animal feeding/growth/reproduction/production have separate but coordinated responsibilities; event order and RNG are deterministic.

## 11. Phase 8 — cross-cutting code review

Apply after the domain changes, not as a huge upfront rewrite.

### Complexity and budget
- Find O(n²) all-pairs and whole-world scans; add spatial indexes only where actual world sizes justify them.
- Bound query result counts, candidate lists, mesh samples/triangles, save bytes and simulation work.
- Avoid full map/vector copies for local changes.
- Cache calculations by revision and invalidate exactly when inputs change.

### State integrity
- Multi-entity operations have preflight, stage, commit and rollback/compensation.
- Events publish after commit.
- Operations are idempotent or explicitly reject duplicates.
- Stale observations and jobs cannot overwrite newer state.
- Restore does not mutate live state until decoded data is validated.

### Algorithm depth
- Target scores use real per-target measurements.
- Support calculations account for cumulative shared loads.
- Flow solvers define physical/gameplay quantities and balance rules.
- Environment updates have one clock/source owner.
- Tools define capability, range, shape and denied outcomes.
- Planners validate full coverage, connection geometry and fit—not just piece count.

### Readability
- Split orchestration by meaningful domain steps.
- Replace magic thresholds with named configuration and units.
- Prefer typed results to sentinel vectors/integers.
- Required-step failure must not be swallowed by continue or converted to success.
- Keep warnings visible and remove misleading indentation.

## 12. Target rating table

Ratings below are the baseline code-quality ratings from the prior audit, not test scores. These are targets to re-evaluate after code changes.

| Mechanic | Current | Target |
|---|---:|---:|
| CoreContracts / shared conventions | 3.5 | ≥4 |
| RuntimeProfiles | 3 | ≥4 |
| WorldScan | 2.5 | ≥4 |
| WorldClassification | 2.5 | ≥4 |
| TerrainVoxel | 3.5 | ≥4 |
| MeshCollision | 3 | ≥4 |
| Excavation | 2.5 | ≥4 |
| PlacementSnap / plank grid | 3 / 2.5 | ≥4 |
| Shoring | 2.5 | ≥4 |
| StructuralIntegrity | 2.5 | ≥4 |
| WorldEnvironment | 2.5 | ≥4 |
| Survival exposure / integration | 2.5 | ≥4 |
| Persistence | 3 | ≥4 |
| Wood cutting commit | 2 | ≥4 |
| Wood structure logic/analysis | 3.5 | ≥4 |
| Fire and heat propagation | 2.5 | ≥4 |
| Raid director / target scorer / spawn | 2.5 / 2 / 2.5 | ≥4 |
| Chunk scheduler | 3 | ≥4 |
| Water flow | 2 | ≥4 |
| Soil/environment integration | 2.5–3 | ≥4 |
| Material M1–M6 | 3 | ≥4 |
| Inventory/crafting | 2.5 | ≥4 |
| Plant growth | 2.5 | ≥4 |
| Animal systems | 2.5 | ≥4 |
| General readability/cost control | 2.5 | ≥4 |

## 13. Four-star source-review checklist

For every mechanic, write down:
1. Authoritative state fields, IDs, units and legal transitions.
2. Algorithm and deliberate approximations.
3. Worst-case input and time/memory growth.
4. Preflight/stage/commit/rollback boundaries.
5. Data ownership and dependency boundaries.
6. Failure categories; no silent success paths.
7. Named validated configuration instead of magic constants.
8. Readability/duplication/side-effect review.
9. Concrete source links and before/after reasoning using the same star rubric.

Testing and UE5.8 verification remain separate scorecards. Tests are needed for project correctness, but passing tests alone cannot make a shallow algorithm a four-star implementation.

## 14. Recommended first implementation batch

Start with these five changes before adding more feature breadth:

1. Make WoodCutterComponent::Finish atomic.
2. Make InventoryComponent::RestoreFromSavedItems staged and secure world-drop/crafting commits.
3. Make world persistence restore and file replacement safe.
4. Correct WaterFlowSolver flow-rate accumulation and define head/volume/elevation quantities.
5. Make StructuralIntegrity aggregate loads across shared supporters.

Then proceed to WorldScan/classification/excavation, mesh/collision/snap, shoring, fire/survival, raid, world scheduling, materials and biological models. This ordering resolves data loss and core algorithm defects before new systems depend on them.
