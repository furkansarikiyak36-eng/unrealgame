# WorldClassification — Specification

## Responsibility
Determine region semantics/capabilities from tagged, authored and inferred evidence. It consumes WorldScan observations but does not own the query engine or terrain edits.

## Canonical classes
- `EditableVoxel`: explicitly eligible for density-based edits.
- `StaticGeometry`: non-editable rock/mesh/terrain.
- `Transition`: boundary between editable volume and static geometry.
- `Water`: water surface/volume classification, not automatically editable terrain.
- `Unknown`: insufficient or contradictory evidence; conservative default.

## Precedence
1. Explicit actor/component capability/semantic tags.
2. Developer-authored manual region/AABB override.
3. Trusted Landscape layer/physical-material markers.
4. Valid existing classification cell.
5. Bounded geometric fallback (limited ray/surface probes).
6. Unknown.

Tie/contradiction behavior is deterministic. Explicit deny beats inferred allow. Slope alone cannot decide soil vs rock.

## Landscape Builder
- Batch-build an authored world or incrementally update dirty regions.
- Read actor/component bounds, collision responses, layer/material data and authored tags.
- Sample height/normal/slope/continuity under a declared budget.
- Produce a versioned map with provenance and reason codes.
- Avoid full-world rescans per frame.
- Manual overrides can be inspected/reset explicitly.

## Query contract
Input: point/bounded region, optional intended capability (dig/snap/support/water) and acceptable revision.
Output: class, capabilities, evidence source/confidence, map revision and rejection reason if denied.

## Safety rules
- Unknown is not EditableVoxel.
- Transition needs an explicit boundary policy; never silently carve across static geometry.
- Class changes invalidate cached authorization based on older map revisions.
- Classification never mutates density or mesh/collision.

## Tests
Precedence, deny-over-allow, AABB boundaries, region-cell edges, rebuild revision, Unknown fallback, transition policy and budgeted dirty updates.

## Status
Portable evidence-priority classification, versioned region-cell map, AABB overrides and provider-driven transactional dirty-cell rebuild exist. The actual Unreal actor/component/physical-material/Landscape probe provider, world sampling policy and broader cache invalidation are still missing. Expanded CI results are pending.
