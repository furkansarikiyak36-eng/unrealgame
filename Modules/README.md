# World Mechanics Modules — Pre-Unreal Source of Truth

**Purpose:** Develop the connected world-interaction system as mechanic-owned modules before Unreal Engine adaptation. The nine phases are development history; they are not the runtime architecture.

## Scope
This track covers runtime contracts/profiles, world scanning and classification, editable terrain, mesh/collision publishing, snap/placement, excavation transactions, shoring, structural support/damage/collapse, survival exposure, world/actor persistence, environmental inputs and cross-module validation.

Existing Unreal code remains in `Source/MyProject2/`. It is a reference/implementation candidate, not proof that the specifications have been ported, engine-neutralized or verified.

## Modules
1. `CoreContracts` — shared IDs, units, errors, revisions and invariant rules.
2. `RuntimeProfiles` — LOW_MID/HIGH_END budgets and session grid settings.
3. `WorldScan` — demand-driven ray/shape/overlap/sample/bounds/physics queries.
4. `WorldClassification` — region classification, source priority and Landscape Builder.
5. `TerrainVoxel` — authoritative base-density + sparse edit state.
6. `MeshCollision` — CPU meshing, revision checks, bounded publish/collision/navigation work.
7. `PlacementSnap` — snap providers, candidate ranking, preview, overlap and commit revalidation.
8. `Excavation` — validated dig/fill operations, atomic edits and change events.
9. `Shoring` — separate physical support-piece definitions, layout planning and placement lifecycle.
10. `StructuralIntegrity` — support graph, load/capacity rules, damage, repair, replacement and collapse.
11. `Survival` — operation costs and underground/environmental exposure integration.
12. `WorldEnvironment` — explicit time/weather/water/moisture/light samples; separate from density.
13. `Persistence` — versioned snapshots, preflight, staging, commit/rollback and actor persistence.
14. `Validation` — contract, unit, integration, round-trip, performance and eventual Unreal gates.

## Canonical documents
- [Mechanics catalog](MECHANICS_CATALOG.md)
- [Manifest/dependencies](ModuleManifest.json)
- [Phase mapping](PHASE_TO_MODULE_MAP.md)
- [End-to-end workflows](INTEGRATION_WORKFLOWS.md)
- [Acceptance matrix](TEST_MATRIX.md)
- [Related systems](RELATED_MECHANICS.md)
- [Integration status](INTEGRATION_STATUS.md)

## Completion rules
A module is not done because a spec or test name exists. Each status needs source files, dependency review, actual test output, unresolved failures, performance constraints and (later) UE 5.8 build/Automation/PIE evidence.

This commit adds the complete specification layer. It does **not** claim that portable implementations exist or Unreal integration occurred.
