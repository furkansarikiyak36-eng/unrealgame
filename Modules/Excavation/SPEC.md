# Excavation — Specification

## Responsibility
Authorize and orchestrate dig/fill as a bounded, repeatable transaction across WorldScan, WorldClassification, TerrainVoxel and MeshCollision.

## Request payload
Operation ID, actor/tool ID, current scan observation, target region/capability, edit shape/transform/dimensions, dig/fill mode, expected classification and terrain revisions, plus caller context.

## Preconditions
- Observation is valid/current and identifies the intended target.
- Region class/capability permits this operation.
- Tool/operation are eligible; range, dimensions, depth/volume and cell count fit current profile.
- Classification and density revisions still match evidence used by request.
- Operation ID is not already committed or in conflict.

## Commit sequence
1. Validate intent and scan freshness.
2. Ask TerrainVoxel to prepare a change set without mutating live state.
3. Recheck region and authoritative revisions.
4. Commit density atomically.
5. Emit event with operation ID, old/new revisions, changed cells/chunks and any rollback receipt.
6. Queue MeshCollision rebuild and invalidate anchors/plans in affected regions.
7. Notify Survival of effort only after successful operation, or follow explicit cancellation/refund policy.

## Failure behavior
Reject static/unknown/transition regions under explicit policy, stale hits, out-of-range requests, invalid dimensions, budget overflow, duplicate IDs and density-preflight failure. Failed operations cannot partially carve terrain, consume material or charge irreversible stamina unless an explicit game rule states so.

## Shape semantics
Sphere/capsule/box are supported request forms. Carve/fill sign, boundary inclusivity and falloff are explicit/tested and shared by preview/commit. No unbounded cell loops.

## Events
TerrainChanged, ExcavationCommitted, ExcavationRejected, DirtyChunksQueued and AnchorsInvalidated are separate typed events or equivalents. Derived mesh failure leaves explicit rebuild debt; it does not rewrite dig success.

## Tests
Every precondition, duplicate op, no-op, dig/fill, rollback, stale scan/map/density revision, denied region, budget cap, dirty-chunk event and derived-publish failure.

## Status
Specified; engine input/animation and collision-driven end-to-end flow remain unverified.
