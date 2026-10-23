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
**Status: IN PROGRESS — wood-cut output staging started.**

Targets: wood cutting; inventory restore/drop/crafting; world persistence and density snapshots; runtime grid/profile identity.

Implementation order:
1. Wood cutting: validate source, result data and slices; stage every output actor with collision/physics disabled; destroy staged actors if a required component or spawn fails; update the source piece only after output preparation succeeds; restore source data/transform/physics if an unexpected staged-output failure occurs during activation.
2. Inventory restore: validate and pack into temporary slot/quick-access state, then swap; do not clear live inventory before validation.
3. World-item drop and crafting: reserve inputs, stage world/output actors, commit item deltas only after all required outputs exist; compensate on failure and make retries idempotent where possible.
4. Persistence: deserialize to a staged store, validate participants and baseline/grid identity, then replace live state in one controlled commit. Use temporary-file write plus atomic replace for durable save files.
5. Runtime profile: separate immutable grid identity from tunable execution budgets; reject incompatible grid changes without blocking budget-only tuning.

**Exit gate:** a required operation either commits all relevant state or leaves the previous state intact; failure reasons are explicit; events fire only after commit.

## Phase 2 — Observation, classification, and world edits

Targets: WorldScan, WorldClassification, Excavation.

- Unify finite-value, normalized-direction, range, shape, ignore-filter, query-channel, result-count and runtime-budget validation.
- Keep raw observations separate from semantic classification; make provenance and stale revisions explicit.
- Update only dirty classification cells and bound candidate generation.
- Require excavation intent to carry the observed target and source/classification revisions; revalidate immediately before edit.
- Make multi-step terrain edits idempotent and rollback-safe.

**Exit gate:** a stale observation cannot authorize a world edit; a failed probe/edit does not partially publish classification or terrain state.

## Phase 3 — Geometry, placement, and structural systems

Targets: TerrainVoxel, MeshCollision, PlacementSnap, Shoring, StructuralIntegrity, wood-structure analysis.

- Cap sample counts, mesh input size, triangle output and queued work; reject malformed dimensions before allocation.
- Keep mesh/collision/navigation derived from authoritative density and stamp every job with source/profile revisions.
- Revalidate snap candidate, overlap, region permission and revisions immediately before placement commit; keep the plank 20 cm placement rule explicit and tested.
- Aggregate structural load across dependents rather than testing every child against the full parent capacity independently.
- Give shoring explicit anchors, coverage and support links; invalidate only affected regions where possible.
- Replace avoidable all-pairs geometry scans with spatial broad-phase filtering, then exact narrow-phase checks.

**Exit gate:** geometry publishes only from current source revisions; placement is atomic; structural capacity includes aggregated loads; costs have documented limits.

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

Remaining in Phase 1: inventory restore/drop/crafting and persistence are not yet changed. The new code has not been verified with UE 5.8 UBT/UHT or in-editor/PIE; do not treat this patch as a passing Unreal build until that verification is run.
