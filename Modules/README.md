# World Mechanics Modules — Pre-Unreal Source of Truth

**Purpose:** Develop the connected world-interaction system as mechanic-owned modules before Unreal Engine adaptation. The nine phases are development history, not the runtime architecture.

## Scope
Runtime contracts/profiles, scan/classification, editable terrain, derived mesh/collision, snap/placement, excavation transactions, shoring, structural support/damage/collapse, survival exposure, environmental inputs, persistence and cross-module validation.

Existing Unreal code remains in `Source/MyProject2/` as a reference/implementation candidate. It is not automatically engine-neutral or verified because this module tree exists.

## Mechanic-based modules
1. `CoreContracts` — IDs, units, typed errors, revisions and invariants.
2. `RuntimeProfiles` — LOW_MID/HIGH_END budgets and session grid policy.
3. `WorldScan` — bounded ray/overlap queries and query-provider contracts.
4. `WorldClassification` — region semantics and Landscape Builder policy.
5. `TerrainVoxel` — authoritative base density and sparse editable overrides.
6. `MeshCollision` — derived mesh/collision/nav revisions and publish gates.
7. `PlacementSnap` — candidate ranking, preview and commit revalidation.
8. `Excavation` — authorized dig/fill operations and edit orchestration.
9. `Shoring` — physical support-piece families and trench planning.
10. `StructuralIntegrity` — support graph, damage, repair and collapse decisions.
11. `WorldEnvironment` — time/weather/water/moisture/light input contracts.
12. `Survival` — effort costs and underground exposure integration.
13. `Persistence` — schema, snapshots, restore staging and rollback.
14. `Validation` — actual tests and evidence gates.

## Documents
- [Complete mechanic catalog](MECHANICS_CATALOG.md)
- [Manifest/dependencies](ModuleManifest.json)
- [Phase migration map](PHASE_TO_MODULE_MAP.md)
- [Connected workflows](INTEGRATION_WORKFLOWS.md)
- [Test/acceptance matrix](TEST_MATRIX.md)
- [Adjacent existing mechanics](RELATED_MECHANICS.md)
- [Current implementation status](INTEGRATION_STATUS.md)

## Status rules
This tree now contains a **partial portable C++ prototype** for several bounded algorithms plus full specifications. It is not the complete mechanics implementation: mesh extraction/publishing, region-map storage/building, engine providers, universal physical-actor adapters and executed tests remain incomplete. No Unreal integration is claimed.

A module graduates only with complete source, dependency review, actual test output, unresolved-failure records and (later) UE 5.8 build/Automation/PIE evidence.
