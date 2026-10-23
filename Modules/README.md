# World Mechanics Modules — Pre-Unreal Source of Truth

**Purpose:** Develop the connected world-interaction system as mechanic-owned modules before Unreal Engine adaptation. The nine phases are development history, not the runtime architecture.

## Scope
Runtime contracts/profiles, scan/classification, editable terrain, derived mesh/collision, snap/placement, excavation transactions, shoring, structural support/damage/collapse, water/soil/fire/survival simulation, raid target/spawn decisions, fair chunk scheduling, M1–M6 material-rule resolution, plant/animal care decisions, transactional crafting preflight, persistence and cross-module validation.

Existing Unreal code remains in `Source/MyProject2/` as a reference/implementation candidate. It is not automatically engine-neutral or verified because this module tree exists.

## Mechanic-based modules
1. `CoreContracts` — IDs, units, typed errors, revisions and invariants.
2. `RuntimeProfiles` — LOW_MID/HIGH_END budgets and session grid policy.
3. `WorldScan` — bounded ray/overlap queries and query-provider contracts.
4. `WorldClassification` — region semantics, versioned cell map, manual overrides and dirty-cell Landscape Builder provider.
5. `TerrainVoxel` — authoritative base density and sparse editable overrides.
6. `MeshCollision` — CPU surface meshing, bounded dirty-chunk work queue, derived mesh/collision/nav revisions and publish gates.
7. `PlacementSnap` — candidate ranking, preview and commit revalidation.
8. `Excavation` — authorized dig/fill operations and edit orchestration.
9. `Shoring` — physical support-piece families and trench planning.
10. `StructuralIntegrity` — support graph, damage, repair and collapse decisions.
11. `WorldEnvironment` — time/weather/water/moisture/light input contracts.
12. `Survival` — effort costs and underground exposure integration.
13. `Persistence` — versioned density snapshot codec, save coordination, restore staging and rollback.
14. `GameplayIntegration` — raid targeting, typed spawn decisions, fair chunk-work scheduling, material-rule precedence/cache, plant growth, animal care/reproduction and staged crafting transactions.
15. `Validation` — actual tests and evidence gates.

## Documents
- [Complete mechanic catalog](MECHANICS_CATALOG.md)
- [Manifest/dependencies](ModuleManifest.json)
- [Phase migration map](PHASE_TO_MODULE_MAP.md)
- [Connected workflows](INTEGRATION_WORKFLOWS.md)
- [Test/acceptance matrix](TEST_MATRIX.md)
- [Adjacent existing mechanics](RELATED_MECHANICS.md)
- [Current implementation status](INTEGRATION_STATUS.md)

## Status rules
This tree contains a **partial portable C++ prototype** for bounded algorithms plus full specifications. The new GameplayIntegration module is still an engine-neutral decision/preflight layer; it does not spawn Unreal Actors or replace the existing in-game component adapters. A CPU surface mesher, revision gate, bounded dirty-chunk queue, versioned region map and provider-driven dirty-cell builder have source code. Unreal world-probe providers, background scheduling, render/collision/navigation publishing, universal physical-actor adapters and broad validation remain incomplete. No Unreal integration is claimed.

A module graduates only with complete source, dependency review, actual test output, unresolved-failure records and (later) UE 5.8 build/Automation/PIE evidence.
