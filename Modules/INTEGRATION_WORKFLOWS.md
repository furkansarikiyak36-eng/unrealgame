# End-to-End Integration Workflows

## 1. Sense → classify
1. Caller issues a bounded query through WorldScan.
2. Provider returns raw facts: hit/miss, target ID, point/normal/distance, raw tags, optional bounds/physics.
3. WorldClassification applies precedence: explicit tags → manual region override → trusted layer/material marker → bounded geometric inference → Unknown.
4. Result includes classification revision/provenance. Unknown is not EditableVoxel.

## 2. Dig → rebuild derived surface
1. Excavation receives tool intent and current scan evidence.
2. Validate capability, range/shape budget, classification revision and terrain revision.
3. TerrainVoxel prepares a change set without mutating live state.
4. Recheck authoritative revisions and commit the density change atomically.
5. Publish the new density revision, changed cells/chunks and invalidated anchors.
6. MeshCollision builds from an immutable bordered snapshot; reject results older than current density revision.
7. Render mesh, collision and navigation publish under separate budgets.

## 3. Place → shoring plan
1. PlacementSnap requests candidates from a piece-specific provider.
2. Filter by capability, current surface revision, orientation, axis, distance and volume overlap.
3. Preview uses the same transformation/rules as commit and explains invalidity.
4. Commit rechecks surface/classification/overlap revisions.
5. Shoring creates pieces/links only after actual placement succeeds.
6. Resource debit/actor spawn must roll back if placement fails.

## 4. Support → damage/collapse
1. StructuralIntegrity consumes explicit nodes and edges; it does not infer graph links from visuals alone.
2. Validate IDs/references/cycles.
3. Compute deterministic game-rule demand/capacity and piece condition.
4. Emit assessment/events; actor/physics adapter applies animation/impulse/removal.
5. Collapse latch prevents duplicate events; terrain changes trigger anchor revalidation.

## 5. Survival update
1. WorldEnvironment provides timestamped temperature, wetness, light, water contact, depth, underground and shelter/heat samples where available.
2. Survival validates freshness and updates exposure using bounded time steps.
3. Successful action events charge costs through the canonical stamina owner.
4. Missing values follow explicit Unknown policy; exposure is saved versionedly.

## 6. Save transaction
1. State-owning modules export immutable versioned snapshots.
2. Persistence validates schema/map/profile/grid, counts, finite values, duplicate IDs and references.
3. Preflight all data before mutating live state.
4. Stage terrain and actors, publish current mesh/collision, restore anchors and rebuild structural graph.
5. Revalidate supports before activating them; restore player/survival through adapters.
6. Commit only when every required participant succeeds. On failure preserve/restore prior state.

## 7. Required vertical slice
Scan eligible soil → classify EditableVoxel → dig bounded trench → density revision changes → mesh/collision catches up → planner proposes support → player commits pieces → graph evaluates support → damage/remove support → deterministic assessment/collapse → save → reload → geometry publishes → supports revalidate → compare canonical state.

## Canonical ownership
- TerrainVoxel alone owns editable density.
- MeshCollision owns derived mesh/collision/navigation revisions.
- WorldClassification owns region semantics, not terrain data.
- WorldScan owns raw observations, not classification authority.
- Shoring owns support-piece lifecycle; StructuralIntegrity owns assessment.
- WorldEnvironment publishes samples; Survival owns exposure calculation.
- Persistence orchestrates snapshots; it does not own gameplay state.
