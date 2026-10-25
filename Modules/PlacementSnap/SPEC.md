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

## Two-anchor endpoints
- The engine-neutral resolver lives at world_mechanics/world_snap_two_anchor_solver.hpp and world_snap_two_anchor_solver.cpp. Its provider sends a bounded set of stable, revision-tagged anchors.
- A candidate pair must be available, unoccupied, target-compatible, support-valid, above the requested load capacity, and current for surface/classification/support revisions.
- Piece local X endpoint offsets define its required span. Pitch comes from the actual endpoint vector, never camera pitch. Gap and end-penetration tolerances are independent.
- The solver is deterministic, bounds candidate and pair counts, and at commit recomputes the exact two-anchor placement to reject disappeared, moved or stale endpoints.
- This portable resolver is a reference implementation for non-Unreal logic. The UE-facing counterpart is Plugins/WorldSnap/Source/WorldSnap/Public/WorldSnapTwoAnchorSolver.h and its Private implementation; maintain behavior parity and do not assume Modules/ source files are automatically linked into the Unreal module.

## Status
A portable candidate filter/ranker, an engine-neutral endpoint solver and a UE-facing two-anchor solver exist. The current ForestCharacter sloped-log path calls the UE-facing solver and now rejects a post-top socket that already carries a beam. Full world candidate providers, live revision adapters, full collision-volume checks and spawn/inventory/support-graph transaction rollback remain incomplete. Tests have been added as source but have not been executed.
