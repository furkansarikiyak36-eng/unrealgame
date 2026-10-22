# World Mechanics Architecture

## Objective
Treat nine phases as a development path toward one connected mechanics system. The permanent layout is mechanic-owned modules under `Modules/`; phase names belong to history and migration records only.

## Canonical ownership
- RuntimeProfiles owns performance/grid policy.
- WorldScan owns raw bounded observations.
- WorldClassification owns region semantics/capabilities.
- TerrainVoxel alone owns editable density.
- MeshCollision owns derived mesh/collision/navigation products and revision stamps.
- PlacementSnap owns candidate selection/validation, not actor lifetime.
- Excavation owns dig/fill orchestration and operation idempotency.
- Shoring owns shoring-plan/piece lifecycle.
- StructuralIntegrity owns graph assessment/damage/collapse decisions.
- WorldEnvironment publishes environment inputs; Survival owns exposure/effort integration but not the canonical player-stat store.
- Persistence coordinates versioned snapshots and atomic restore.
- Validation owns tests/evidence, not gameplay rules.

## Engine boundary
Portable modules must not depend on Unreal headers, UObject, Actor, Component, World, Blueprint, Dynamic Mesh, Chaos or SaveGame APIs. Define ports around queries, mesh publishing, overlap, actor creation, time, storage and character stats. Later Unreal adapters implement those ports.

Existing `Source/MyProject2/` remains in place until extracted code has parity tests. Source there is not engine-neutral because a specification exists.

## State authority and derived products
Terrain density is authoritative. Render mesh/collision/navigation are derived from revisioned snapshots. Shoring pieces are separate physical entities. The structure graph represents explicit validated connections, not visual proximity alone. Region classification and environment layers are separate from density.

## Transaction rule
Validate → stage → commit → emit events. If any authoritative commit fails, preserve previous state or compensate from a receipt. Derived mesh failure creates rebuild debt; it must not silently corrupt or overwrite terrain state.

## Completion evidence
See `Modules/INTEGRATION_STATUS.md`. No Unreal integration claim is valid until the target adapter builds and relevant Automation/PIE flows run and are recorded.
