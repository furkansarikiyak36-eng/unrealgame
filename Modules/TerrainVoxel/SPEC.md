# TerrainVoxel — Specification

## Responsibility
Own all authoritative editable terrain density. Meshes, collision, anchors and saved visual assets are derived consumers, not a second density owner.

## Model
- Base source: Landscape heightfield density or deterministic flat/procedural source.
- Sparse override/delta: only edited density/material cells relative to base.
- Addressing: world position → density cell → chunk; spacing/dimensions come from RuntimeProfiles.
- Density convention: 0 empty, 1 solid; boundary/interpolation policy is fixed and tested.
- Snapshot: immutable bordered density/material block + source revision; normally at least a one-cell meshing border.

## Edit shapes
Sphere, capsule and box. Request includes transform/center, dimensions, operation (dig/fill/set), material policy, operation ID and expected revision. Extents/sample count/affected-cell count are bounded.

## Transaction semantics
1. Validate input/revision/bounds and calculate affected cells/chunks.
2. Build a change set without mutating authoritative state.
3. Validate budget and operation preconditions.
4. Commit all overrides as one revision.
5. Return before/after values or rollback receipt plus exact dirty chunk/neighborhood set.
6. Failed operation leaves density and revision unchanged.
7. Duplicate operation IDs are idempotent or rejected consistently.

## Chunk and seam rules
- Chunk coordinate conversion uses floor semantics for negative coordinates.
- Mark border/neighbor chunks dirty where surfaces cross seams.
- Grid spacing cannot change in a saved world without explicit migration.
- Sparse entries equal to base values may be pruned only if round-trip semantics remain identical.

## Materials
Density and surface/material ID are related but distinct channels. Material source/override policy is explicit. Unknown material stays Unknown; it is not auto-converted to soil.

## Persistence
Export sorted/deterministic sparse records plus grid/profile/base-source identity and revision. Import validates sizes, finite values, coordinate ranges and duplicates before swap. Generated meshes are not canonical terrain payloads.

## Tests
Negative chunk coordinates, base fallback, dig/fill inverse where applicable, shape bounds, one-cell border, deterministic export, stale revision, duplicate operation, rollback and seam invalidation.

## Status
Specified. Existing Unreal draft has density/snapshot APIs, but this is not yet an extracted portable implementation.
