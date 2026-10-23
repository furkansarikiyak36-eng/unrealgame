# Five-Phase Plan — Raise Mechanics Code Quality to at Least Four Stars

**Target:** every mechanic currently rated below ★★★★☆ reaches at least ★★★★☆ source-code quality.  
**Scope:** state correctness, algorithm depth, responsibility boundaries, performance complexity, deterministic behavior, failure handling, and maintainability. This is not a claim that every system is physically exact or already verified in Unreal Engine 5.8.

## Working rules

- Work in small, reviewable patches. Do not mass-rewrite unrelated systems in one commit.
- Keep the current implementation as the source of truth until a replacement passes its acceptance gate.
- For state-changing operations, define preconditions, staging, commit, compensation/rollback, and event timing.
- Bound both work and memory for hot loops and bulk input.
- Distinguish pure portable C++ verification from Unreal Build Tool / Header Tool / Automation / PIE verification.
- Ratings are code-quality assessments, not test-count ratings. Do not assign four stars merely because tests pass.

## Phase 1 — State integrity and atomic transactions
**Status: IN PROGRESS — wood-cut output staging and staged inventory restore implemented; UE validation pending.**

Targets: wood cutting; inventory restore/drop/crafting; world persistence and density snapshots; runtime grid/profile identity.

Implementation order:
1. Wood cutting: validate source, result data and slices; stage every output actor with collision/physics disabled; destroy staged actors if a required component or spawn fails; update the source piece only after output preparation succeeds; restore source data/transform/physics if an unexpected staged-output failure occurs during activation.
2. Inventory restore: validate and pack into temporary slot/quick-access state, then swap; do not clear live inventory before validation.
3. World-item drop and crafting: reserve inputs, stage world/output actors, commit item deltas only after all required outputs exist; compensate on failure and make retries idempotent where possible.
4. Persistence: deserialize to a staged store, validate participants and baseline/grid identity, then replace live state in one controlled commit. Use temporary-file write plus atomic replace for durable save files.
5. Runtime profile: separate immutable grid identity from tunable execution budgets; reject incompatible grid changes without blocking budget-only tuning.

**Exit gate:** a required operation either commits all relevant state or leaves the previous state intact; failure reasons are explicit; events fire only after commit.

## Phase 2 — Observation, classification, and world edits

**Status: portable-module hardening implemented; engine-provider integration remains open.**

The first implementation batch is on the separate `modules/mechanics-pre-unreal` branch in [PR #19](https://github.com/furkansarikiyak36-eng/unrealgame/pull/19), commit `89fd0a626859dd9846465deddaf5ca62842a816f`. [Its portable CMake build and CTest pass](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/39881334477). Because PR #19 is not merged into `main`, these module changes are not yet part of the main branch.

Targets: WorldScan, WorldClassification, Excavation.

- Unify finite-value, normalized-direction, range, shape, ignore-filter, query-channel, result-count and runtime-budget validation.
- Keep raw observations separate from semantic classification; make provenance and stale revisions explicit.
- Update only dirty classification cells and bound candidate generation.
- Require excavation intent to carry the observed target and source/classification revisions; revalidate immediately before edit.
- Make multi-step terrain edits idempotent and rollback-safe.

**Exit gate:** a stale observation cannot authorize a world edit; a failed probe/edit does not partially publish classification or terrain state.

**Current batch completed:** WorldScan now uses common query budgets, validates hit revision/normal data, applies ignored-target filters consistently, and orders hits deterministically. Classification upserts stage the dirty batch and use a touched-cell rollback path instead of copying the whole map. Excavation requires a live observation-validation callback and rejects source-revision drift before changing density. Regression coverage was added for query caps, ignored targets, classification storage limits, and stale excavation hits.

**Still open before Phase 2 is fully closed:** implement/connect the actual Unreal query provider and Landscape/physical-material evidence provider; integrate a real current-target requery at the UE gameplay boundary; run UE5.8 UBT/UHT and Editor/PIE checks. Passing the portable-module CI alone does not close those engine-integration items.

## Phase 3 — Geometry, placement, and structural systems

**Status: portable mesh/placement/structural changes implemented; CI verification pending.**

The current Phase 3 source changes are being developed on `modules/mechanics-pre-unreal` under [PR #19](https://github.com/furkansarikiyak36-eng/unrealgame/pull/19). The roadmap branch remains separate from that module branch; this document records scope and status only.

Targets: TerrainVoxel, MeshCollision, PlacementSnap, Shoring, StructuralIntegrity, wood-structure analysis.

- Cap sample counts, mesh input size, triangle output and queued work; reject malformed dimensions before allocation.
- Keep mesh/collision/navigation derived from authoritative density and stamp every job with source/profile revisions.
- Revalidate snap candidate, overlap, region permission and revisions immediately before placement commit; keep the plank 20 cm placement rule explicit and tested.
- Aggregate structural load across dependents rather than testing every child against the full parent capacity independently.
- Give shoring explicit anchors, coverage and support links; invalidate only affected regions where possible.
- Replace avoidable all-pairs geometry scans with spatial broad-phase filtering, then exact narrow-phase checks.

**Exit gate:** geometry publishes only from current source revisions; placement is atomic; structural capacity includes aggregated loads; costs have documented limits.

**Implemented in the current Phase 3 batch:**
- CPU surface meshing rejects density grids over an explicit input-sample cap before scanning all samples or constructing the edge cache.
- Degenerate triangles are skipped before consuming triangle budget; mixed-material tetrahedra use a deterministic majority-material policy with lowest-ID tie break.
- Placement snapping bounds candidate count, validates dot-threshold ranges, uses strict deterministic distance ordering and checks region permission again at commit time.
- Adds a separate opt-in 20 cm plank-grid helper so the plank grid does not implicitly alter log/socket or terrain-tool placement.
- Structural assessment aggregates dependent loads upstream over active support edges, splits load deterministically across multiple incoming supports, and exposes the aggregate demand/ratio in assessment output.

**Remaining:** local UE Dynamic Mesh/collision/navigation publication and stale-product rejection at the engine adapter; actual oriented-volume overlap provider; physical shoring anchors, connected assembly coverage and region-local invalidation. CI for the current source commit must pass before this batch is treated as portable-code verified.

## Phase 4 — Environment, water, soil, fire, and survival

Targets: WorldEnvironment, WaterFlowSolver, soil integration, fire/heat, Survival.

- Give water an explicit cell volume/capacity, terrain head and flow rate model; aggregate all outgoing/incoming rates and enforce mass-balance tolerance.
- Use bounded stable substeps or a fixed-step accumulator without discarding elapsed time.
- Define one authoritative owner for water, soil, temperature, ignition, fuel and exposure state; other systems consume snapshots/events.
- Replace repeated full-world fire scans with radius/region queries and cached candidate sets invalidated by movement.
- Put maximum work per tick on catch-up paths and define what happens when a simulation budget is exhausted.
- Make environmental samples carry source identity, revision, sample time, validity and staleness.

**Exit gate:** state ownership is unambiguous; conservation and time-step behavior are tested; no unbounded tick loop or redundant whole-world scan remains in the targeted paths.

## Phase 5 — Gameplay systems, integration, and performance pass

Targets: Raid Director/targeting/spawn; chunk scheduler; material M1–M6; plants, animals, crafting integration; cross-cutting readability and budgets.

- Score targets from actual threat/accessibility features, not constant weight additions. Use deterministic, seedable random sources where reproducibility matters.
- Return typed spawn outcomes instead of sentinel vectors; treat missing navigation/query providers as explicit policy, not automatic success.
- Ensure chunk scheduling is fair under budgets and that read operations do not mutate dirty state.
- Specify material rule precedence, cache invalidation and preflight/commit boundaries.
- Move species/crop/feed/production parameters into validated data definitions; make growth/feed/reproduction decisions deterministic and order-independent when required.
- Run the final complexity/readability pass and ensure every below-four item has a source-review record, known limitations and a regression test.

**Exit gate:** each targeted mechanic has a four-star source-review checklist, regression evidence, explicit unsupported cases and a declared verification level.

## Phase 1 implementation record

### Patch 1 — Wood cutting
Files changed:
- `Source/MyProject2/Wood/WoodCutterComponent.cpp`
- `Source/MyProject2/Wood/WoodCutterComponent.h`

Changes:
- Validate every result piece and slice before spawning outputs.
- Prepare spawned pieces with collision and simulation disabled.
- Clean up staged actors if spawning or required component validation fails.
- Delay source-piece mutation until all output pieces have been prepared.
- Preserve source transform, collision and velocities for rollback/commit handling.
- Document `ReshapeVisual` as a visual-only callback to avoid gameplay side effects during the transaction.

### Patch 2 — Inventory restore
File changed:
- `Source/MyProject2/InventoryCore/InventoryComponent.cpp`

Changes:
- Build restored slot state in a temporary array; do not clear or partially fill the live inventory during validation.
- Preserve locked slot contents and slot flags as the old clear behavior intended.
- Validate item IDs/counts, stack limits, finite non-negative weights and total resulting weight.
- Pack stacks against staged state, then replace live slots in one assignment.
- Emit one inventory-changed notification only after commit, rather than exposing intermediate clear/add states.

Remaining in Phase 1: world-item drop/crafting transaction coordination, persistence staged replacement/atomic file save, and runtime grid/profile identity are not yet changed. The code has not been verified with UE 5.8 UBT/UHT or in-editor/PIE; do not treat this patch as a passing Unreal build until that verification is run.
