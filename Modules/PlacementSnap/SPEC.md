# PlacementSnap — Specification

## Responsibility
Resolve a valid candidate transform/anchor for a piece and revalidate it at commit time. It does not own terrain density or shoring actor lifetime.

## Candidate inputs
Source observation and normal; region class/capabilities/revision; surface anchor ID and mesh/collision revision; piece definition (footprint, allowed faces/orientations, sockets, axis tolerance, offset, collision shape, grid provider); nearby obstacles through an overlap provider.

## Candidate pipeline
1. Request candidates from a piece-specific provider.
2. Filter invalid capability, stale surface, unsupported face, excessive distance, incompatible axis and inaccessible target.
3. Calculate candidate transform/rotation/offset and align to surface normal within configured angle.
4. Apply type-specific grid/socket rule (e.g. configurable 0.20 m plank grid); do not hard-code spacing in WorldScan.
5. Validate full candidate volume/clearance.
6. Rank deterministically: validity → explicit socket priority → alignment error → distance → stable candidate ID.
7. Return preview transform plus detailed rejection reasons.
8. At commit, reread surface/classification/overlap revisions; changed evidence returns StaleRevision and asks caller to preview again.

## Atomic placement
Return a validated decision/receipt. The caller coordinates spawn, inventory debit and graph registration. If one step fails, remove any staged object and refund/rollback the debit; never leave partial placement.

## Rules
Preview and commit share the same functions. A plausible ghost is not proof that commit remains legal. Persistent anchors require post-load revalidation.

## Tests
Normal alignment, axis tolerance, max distance, stable tie-break, 20 cm grid, revision invalidation, full-volume overlap, forbidden region, preview/commit parity and partial-commit rollback.

## Status
Specified; Unreal trace/overlap/ghost adapters remain unverified.
