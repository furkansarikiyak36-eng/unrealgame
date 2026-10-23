# Unreal Engine 5.8 — incremental integration

## Why this is staged

Do not drag the entire portable `Modules/` tree into `Source/MyProject2/` or copy all headers by hand. The portable implementation is kept in its current CMake-tested location. The exporter copies one layer at a time into a single Unreal plugin:

`YourProject/Plugins/WorldMechanics/Source/WorldMechanicsRuntime/`

All imported algorithms are compiled in **one Unreal module**. This avoids cross-module DLL export macros for engine-neutral C++ classes and keeps the plugin's dependency setup small. The plugin's source is cumulative: Phase 1 adds files to Phase 0; Phase 2 adds to Phases 0–1, and so on. The exporter will not delete existing plugin files or silently replace files that differ from the portable source.

## Add one phase at a time

Run these commands from the repository checkout. Replace the project path with the directory containing your `.uproject` file:

```powershell
.ToolsUnrealIntegrationExport-Phase.ps1 -Phase 0 -ProjectRoot "D:\Games\MyProject2"
```

Then regenerate project files and build the UE 5.8 Editor target. Fix any compiler error before adding the next layer. Repeat in order:

```powershell
.ToolsUnrealIntegrationExport-Phase.ps1 -Phase 1 -ProjectRoot "D:\Games\MyProject2"
.ToolsUnrealIntegrationExport-Phase.ps1 -Phase 2 -ProjectRoot "D:\Games\MyProject2"
.ToolsUnrealIntegrationExport-Phase.ps1 -Phase 3 -ProjectRoot "D:\Games\MyProject2"
.ToolsUnrealIntegrationExport-Phase.ps1 -Phase 4 -ProjectRoot "D:\Games\MyProject2"
.ToolsUnrealIntegrationExport-Phase.ps1 -Phase 5 -ProjectRoot "D:\Games\MyProject2"
```

The script checks that the project root contains a `.uproject`, verifies the phase order using `WorldMechanicsIntegration.json`, and records which layers were imported. It uses a staging directory and checks for name/content collisions before copying files. It never rewrites the original `Modules/` source tree.

## Import order

| Phase | Portable modules added | What is in the layer |
|---|---|---|
| 0 — Core | `CoreContracts`, `RuntimeProfiles` | IDs/revisions, vectors, result/error contracts and performance profiles |
| 1 — World query | `WorldScan`, `WorldClassification` | Query abstractions, region classification, classification map |
| 2 — Terrain | `TerrainVoxel`, `MeshCollision`, `PlacementSnap` | Density edits, CPU mesh data, dirty/revision gates, snap planning |
| 3 — Construction | `Excavation`, `Shoring`, `StructuralIntegrity` | Authorized edit preflight, shoring plans, support/load/collapse decisions |
| 4 — Environment | `WorldEnvironment`, `Survival` | Water/soil/fire simulation and survival exposure calculations |
| 5 — Persistence/gameplay | `Persistence`, `GameplayIntegration` | Snapshot codec/restore transaction, raid/spawn decisions, chunk scheduling, material rules, plant/animal models, crafting preflight |

## Important boundary

This exporter makes the portable C++ algorithms available as a UE runtime plugin module. It does **not** convert pure C++ models into live Unreal Actors, UObjects, Components, WorldSubsystems, navigation queries, rendering/collision publication, world-drop transactions or editor tools. Those adapters need to be added and tested as separate work against the existing `Source/MyProject2/` gameplay code.

After every phase:

1. Regenerate project files if the IDE needs it.
2. Build `MyProject2Editor` with Unreal Engine 5.8.
3. Inspect compiler errors/warnings and run the relevant test or small test map.
4. Only then export the next phase.

The existing CMake test suite remains useful for portable-domain regressions but is not a substitute for UBT/UHT, Unreal Automation, PIE, or Unreal Insights verification.

## Safety and recovery

- Exporting Phase 0 fails if `Plugins/WorldMechanics` already exists; the script does not overwrite an existing plugin.
- Phases 1–5 can only be added in order.
- A collision with an existing file of different content is an error, not an overwrite.
- To restart from zero, back up the plugin and remove `Plugins/WorldMechanics` yourself; the original portable source remains under `Modules/`.
