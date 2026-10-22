# Shoring — Specification

## Responsibility
Plan, place and manage physical excavation support. A shoring piece is a separate physical entity, never part of voxel mesh.

## Piece families
- Vertical timber upright/post.
- Wall-facing lagging/waling board.
- Cross strut between opposing supports.
- Base/seat/sole piece.
- Temporary platform and ladder/access piece.
- Optional wedges/ties/repair pieces.

Definition data: stable type ID, nominal dimensions, mass, material profile, allowed anchor types, durability/health, collision footprint, placement constraints and build/repair cost policy.

## Planner contract
Inputs: trench/shaft/room bounds, axis, width, depth, terrain/surface observations and revisions, piece catalog, configurable station spacing, edge clearance and output budget. Output: stable plan ID/revision, stations, candidate transforms, piece types, anchor/socket links, unsupported segments and validation issues. Planner proposes; it does not spawn actors.

## Layout rules
- Station spacing is configurable; 1.5 m center-to-center is a prototype reference, not a safety standard.
- Endpoint policy is explicit: short final bay or redistributed spacing.
- Opposing uprights and matching cross-strut anchors are generated for supported plan types.
- Boards align to local surface/normal samples where available.
- Reject invalid width/depth, huge piece count, missing samples, severe surface inconsistency and known collisions.
- Report incomplete coverage; never label a plan complete with missing required parts.

## Lifecycle
Proposed → Previewed → Validated → CommitRequested → Installed, or Rejected/NeedsRevalidation/Removed/Damaged/Failed.
- Preview does not mutate world state.
- Commit revalidates terrain, classification and overlap revisions.
- Spawn/connection failure compensates already-created pieces.
- Terrain edits invalidate affected plans/anchors.
- Carry, drag, repair, replace and remove update support graph and persistence consistently.

## Rules
Same inputs/revisions produce deterministic plan. Normal mode requires explicit confirmation; auto-placement may exist as a dev/test mode. Dimensions/capacity values are gameplay parameters, not real-world engineering guidance.

## Tests
Station endpoints/spacing, max count, narrow/wide/deep bounds, missing surfaces, stale revisions, invalid links, collision issues, partial spawn rollback, remove and repair/replace lifecycle.

## Status
Specified; physical fit/actor lifecycle in UE remains to be verified.
