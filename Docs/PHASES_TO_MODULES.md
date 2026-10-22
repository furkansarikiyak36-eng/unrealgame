# Phase History → Mechanics Modules

Phases remain useful as historical checkpoints, but are not the target directory structure.

| Existing phase/work area | Long-lived responsibility |
|---|---|
| Phase 0 — baseline audit | Architecture and migration inventory |
| Phase 1 — runtime contracts | CoreContracts |
| Phase 2 — scan/classification | WorldScan |
| Phase 3 — smooth voxel | TerrainVoxel |
| Phase 4 — scan-to-dig/placement | Excavation |
| Phase 5 — shoring planner | Shoring |
| Phase 6 — support/damage/collapse | StructuralIntegrity |
| Phase 7 — survival integration | Survival |
| Phase 8 — world persistence | Persistence |
| Phase 9 — vertical slice | Validation and end-to-end scenario coverage |

This mapping is responsibility-based. It does not mean existing source folders can be renamed safely without updating include paths, ownership, build rules and tests together.
