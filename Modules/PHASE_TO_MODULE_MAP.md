# Phase-to-Module Map

Phases are historical checkpoints. Modules are the permanent mechanic ownership layout. A phase can contribute to multiple modules; a module can receive work from multiple phases.

| Phase | Original goal | Destination modules | Caveat |
|---|---|---|---|
| 0 | Baseline audit and owner decisions | CoreContracts, Validation, architecture docs | Static audit only; clean UE build/test is a separate gate. |
| 1 | Runtime contracts and profile spine | CoreContracts, RuntimeProfiles | Existing Unreal contract code must be extracted behind portable types. |
| 2 | World scan/classification | WorldScan, WorldClassification | Engine query adapters and landscape/tag behavior need real UE tests. |
| 3 | Smooth voxel density/mesh/collision | TerrainVoxel, MeshCollision | Density, mesher, chunk actor/queue are drafts; pipeline remains unverified. |
| 4 | Scan-to-dig and surface placement | Excavation, PlacementSnap, WorldScan, TerrainVoxel, MeshCollision | Atomic edit and stale-surface rules need end-to-end verification. |
| 5 | Shoring planner/pieces | Shoring, PlacementSnap | Physical actor fit and lifecycle require UE validation. |
| 6 | Support/damage/collapse | StructuralIntegrity, Shoring | The load/capacity model is a gameplay approximation. |
| 7 | Survival integration | Survival, WorldEnvironment | Environmental samples and character adapters need integration tests. |
| 8 | World save/load | Persistence plus state-owning modules | Full save/load transaction remains an external validation gate. |
| 9 | Vertical slice/stone persistence | Persistence, Validation | Stone persistence is not universal physical-actor persistence. |

## Migration rule
Do not rename Unreal source files into Modules and call them engine-neutral. Extract domain rules behind portable types/ports, add parity tests, then build Unreal adapters. Preserve `Source/MyProject2/` until parity and regression evidence exists.
