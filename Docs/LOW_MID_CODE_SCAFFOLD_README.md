# Low–Mid Code Scaffold — UE 5.8 Setup and Validation

**State:** Source scaffold prepared on `implementation/low-mid-code-scaffold`. This is not evidence of a successful UE 5.8 build or Editor/PIE run.

## What is added

- Reuses the existing sparse excavation edit/snapshot data layer.
- Adds a LOW_MID/HIGH_END runtime profile settings asset via `UWorldRuntimeProfileSettings`.
- Adds actor/tag/bounds-based first-pass Landscape classification.
- Adds a Landscape heightfield-to-density provider component.
- Adds cell-level edit preflight against the Landscape classification map.
- Adds a sample-budgeted chunk surface generation queue using Marching Tetrahedra and `UProceduralMeshComponent`.
- Routes the player's ordinary view trace through `WorldScanSystem::TraceWithParams` without removing the existing placement ghost ignores.
- Adds a first shoring layout generator for uprights, boards and struts, with anchors and terrain-change invalidation.
- Extends the existing survival component with ambient temperature/wetness exposure fields and state export/import.
- Adds a SaveGame adapter for sparse excavation state, shoring records and player/survival state.
- Adds Blueprint-callable `AVoxelPrototypeDirector` and automation test definitions.

## Cross-system integration added in this pass

- The existing player `ShovelStroke` is the canonical excavation input. It now calls `UVoxelEditService` for both carve and fill, preserving the current loose-soil sack accounting rather than introducing a second shovel mechanic.
- Dig/fill cost stamina through `UExcavationSurvivalAdapterComponent`; if payment fails, the voxel transaction is rolled back.
- View traces in `AForestCharacter::TraceFromView` now route through the shared Scan API while keeping the existing placement-ghost ignore rules.
- `LogSnapSystem::Resolve` now delegates candidate selection to the linked `WorldSnap` resolver.
- `UWorldMechanicEventSubsystem` provides a bounded, prioritized event channel. Classification, voxel edit/rollback/reset, mesh publish, shoring damage/break, shoring invalidation, survival exposure and save/load completion publish events.
- Save/load validates survival data before changing live state; voxel density deltas and shoring state are staged and rollback is attempted if staging fails.

## Test-map setup

1. Build/open the project in Unreal Engine **5.8** after checking out this branch. Confirm the `ProceduralMeshComponent` plugin is installed for that engine build.
2. Make a dedicated test map. Add a Landscape actor that is intended to be a voxel-editable test patch.
3. Add `ULandscapeDensityProviderComponent` to that Landscape actor. It samples the Landscape collision heightfield and registers as the base-density source.
4. Mark mountains/rock actors with actor or component tag `Terrain.Mode.StaticGeometry` (or `World.Object.Rock`). Mark explicit editable mesh/regions with `Terrain.Mode.VoxelEditable`; the default Landscape classification is editable when `bTreatLandscapeAsEditableByDefault` is true.
5. Place one `AVoxelPrototypeDirector` in the test map. Call `BuildWorldClassification` after the map is ready.
6. In the LOW_MID profile, call `RebuildRegion` over a small test patch to create the initial voxel surface representation. Then call `CarveSphere` or `CarveTunnel`; each committed voxel edit queues its affected chunks automatically.
7. Call `BuildTrenchPlan` with explicit endpoints and dimensions; inspect the returned `LastTrenchPlan` candidates in Blueprint/debug tooling; call `CommitLastTrenchPlan` only after the voxel surface and anchors have passed validation.
8. Exercise `SaveWorldToSlot` and `LoadWorldFromSlot` on `UWorldPersistenceAdapter` after the terrain, mesh and support systems have initialized.

## Known limits — do not mistake this scaffold for a finished game system

- The original Landscape component has **not yet been safely masked/removed under every generated voxel chunk**. Use a small dedicated test patch; do not deploy over a large production Landscape until visible terrain and collision ownership are solved.
- The density source is a heightfield adapter to Landscape. It cannot infer closed 3D caves, undercuts or arbitrary solid-mesh density by itself. Static rock remains protected; caves/tunnels inside solid rock need an explicit base-density source or region policy.
- Classification currently uses actor/component tags, Landscape type and actor bounds. Material-layer weight sampling and a rich editable map UI are still future work.
- Smooth mesh generation and collision are a prototype backend. Chunk seams, visual materials, partial Landscape masking, save/load and actual physics contact must be verified in UE 5.8.
- The shoring planner generates separate cube-mesh actor placeholders, not final art assets. Surface support and connection rules are initial gameplay checks, not real-world geotechnical engineering calculations.
- Survival environmental values must be supplied by a region/weather system. This scaffold provides the receiving API and save-state methods; it does not yet implement full water/temperature/underground simulation.
- The automation tests are definitions only. They have not been run in Unreal Editor/UBT in this session.

## First acceptance scenario

Use one small Landscape area and a tagged static rock:

1. Classification reports Landscape as `EditableVoxel` and rock as `StaticGeometry`.
2. A carve wholly inside the editable region succeeds.
3. A carve whose touched density cells include static rock or Unknown terrain is rejected before changing the density snapshot.
4. The dirty chunk queue publishes a smooth mesh/collision over subsequent frames without a whole-world rebuild.
5. A trench preview produces uprights, wall boards and struts; a valid layout commits as separate actors.
6. Removing/altering a voxel surface invalidates affected anchors for recheck.
7. Save/reload restores logical voxel deltas, shoring records, player transform and survival state.
8. Run the same tests in Low–Mid, profile and stress it before enabling High-End.

## Performance interpretation

`LowMidMaxDensitySamplesPerTick` and `MaxChunksPublishedPerTick` cap work per frame, but they do not guarantee a target FPS. Measure Game Thread time, sample-cache misses, mesh generation time, collision update time, active/dirty chunks and worst-frame in Unreal Insights. The HIGH_END setting is a later quality/performance profile; the first complete validation must be LOW_MID.
