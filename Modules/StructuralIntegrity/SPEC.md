# StructuralIntegrity — Specification

## Responsibility
Own the authoritative support graph and deterministic game-rule assessment for shoring/structure. Physics may enact a result but is not the only authority.

## Graph records
Node: stable entity ID, kind/material profile, self-weight or load demand, capacity, health/durability, support state, bounds reference and collapse latch. Edge: stable edge ID, endpoint IDs, relation/socket/contact type, capacity/condition and active flag.

## Graph validation
- No duplicate node/edge IDs.
- Every edge references valid distinct endpoints.
- Dangling links are removed/rejected explicitly when a node is replaced/deleted.
- Cycle policy is defined per relation type; unsupported cycles are reported, not allowed to hang evaluation.
- Traversal/tie-breaking is stable for deterministic results.

## Assessment
- Demand/capacity ratio and safety margin are configurable gameplay rules.
- Missing material/capacity data returns Unknown/Invalid, not infinite strength.
- Damage/durability can reduce effective capacity or deactivate edges.
- Support loss finds the affected subgraph and recomputes it.
- Return Supported/Strained/Unstable/Unsupported with reasons, affected IDs, graph revision and proposed events.
- Emit events; do not destroy actors during graph calculation.
- Collapse latch makes decision idempotent and avoids repeated collapse impulses after reload.

## Damage/repair/replace
Damage includes amount/source/reason and optional direction. Repair validates health caps/material cost and increments graph revision. Replacement transfers only explicitly compatible anchor/link data; stale edges are not retained implicitly.

## Cross-module interaction
Shoring owns piece and plan lifecycle. StructuralIntegrity validates links/evaluates graph. Terrain changes invalidate anchors; PlacementSnap/Shoring revalidate. Persistence restores links but supports remain NeedsRevalidation until collision/surface revisions are current.

## Safety statement
This is a deterministic gameplay approximation, not certified structural or geotechnical engineering.

## Tests
Duplicate IDs, dangling edge, cycle policy, stable evaluation order, capacity boundary, missing capacity, damage threshold, repair cap, node removal, cascading support loss and collapse latch.

## Status
Specified; Unreal Chaos/physics consequences and broad structure-system equivalence require verification.
