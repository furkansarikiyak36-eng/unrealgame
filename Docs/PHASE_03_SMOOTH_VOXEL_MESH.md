# Phase 3 — LOW_MID smooth voxel density, mesh and collision

**Branch:** `feature/phase-03-lowmid-smooth-voxel`  
**Base:** `feature/phase-02-scan-classification-lowmid`  
**Status:** Implementation draft; Unreal Engine 5.8 build, Automation tests and PIE validation are still required.  
**Scope:** One shared runtime implementation, beginning with the LOW_MID profile. This phase does not merge changes into `main`.

## What was added

### 1. One authoritative density store

`UExcavationVolumeSubsystem` now implements `IVoxelDensitySource`. Sparse carve/fill overrides stay authoritative in that subsystem; the renderer never keeps a second editable copy of density.

The subsystem provides:
- base-density sampling (before sparse overrides);
- final density sampling (sparse override first, then active/fallback base provider);
- profile grid configuration that refuses to reinterpret existing sparse coordinates;
- bordered chunk density snapshots with a monotonic source revision;
- per-sample base material IDs in the snapshot;
- revision invalidation for edits, rollback, imports and base-source changes.

Density convention is normalized: **0 = empty, 1 = solid, ISO = 0.5**. Snapshot values and metadata are validated before meshing. A one-sample border is included on every side so neighboring chunks use the same boundary samples.

### 2. Smooth CPU surface extraction

`FWorldMarchingTetrahedraMesher` implements `IVoxelSurfaceMesher` as a pure CPU operation. Each cell cube is split into six consistently oriented tetrahedra. The mesher:
- interpolates the ISO surface along density-crossing edges;
- reuses vertices for shared edges and exact ISO-level grid nodes;
- estimates smooth normals from density gradients, with a finite fallback for zero gradients;
- emits local-centimetre vertices, normals, metre-scaled planar UVs and a material ID per triangle;
- rejects non-finite/out-of-range density, malformed material arrays, invalid metadata and triangle-budget overflow;
- validates the resulting triangle/index/attribute buffers before returning output.

The current triangle cap in the mesher is 1,000,000 triangles per chunk; each chunk actor applies its stricter configurable `MaxGeneratedTriangles` limit before publication.

### 3. Dynamic mesh and collision publication

`AExcavationTerrainChunkActor` now separates:
1. density snapshot capture;
2. CPU mesh generation;
3. game-thread Dynamic Mesh conversion and publication;
4. collision and navigation update.

The component is tagged `World.Object.VoxelSurface` for the Phase 2 scan/support rules. Mesh output is published only if its source revision still matches authoritative density; stale worker output is discarded and the chunk is re-queued. Conversion failures do not publish a partially built candidate mesh.

Base Landscape density uses a continuous one-cell-width ramp around the heightfield surface. That allows the mesher to interpolate the surface instead of simply placing every surface vertex at a sample-cell centre. Landscape height columns and mapped physical-material IDs are cached with a bounded cache size.

### 4. Runtime profile and scheduling

The chunk subsystem initializes the selected runtime settings, configures the excavation grid and locks the session profile before edits can reinterpret grid coordinates. With no profile asset configured, the current fallback remains LOW_MID.

For LOW_MID the initial profile values are:
- 25 cm sample spacing;
- 32 cells per axis and an 8 m chunk;
- one background mesh job at a time;
- at most one mesh publication and collision update per frame;
- a bounded mesh-job/dirty queue and at most 32 managed active chunks.

The dirty queue coalesces repeated requests. Overflow triggers a bounded, incremental full refresh rather than allowing an unbounded burst. The worker pool only reads an immutable density snapshot and produces plain CPU arrays. UObjects, Dynamic Mesh components, collision and navigation are touched on the game thread. Completed output is also bounded by the selected mesh-job budget, preventing publication backlog from growing without limit.

Async collision cooking is selected from the active profile setting; LOW_MID keeps it off initially. These values are implementation starting points, not benchmark results. HIGH_END must still go through its later dedicated validation phase.

## New Automation coverage

Added to the phase runner:
- `MyProject2.World.VoxelMesher.HorizontalPlane` — ISO height, triangle generation, shared-edge reuse, upward smooth normals, material ID count and source revision.
- `MyProject2.World.VoxelMesher.ExactIsoNodesStayConnected` — exact 0.5 density samples share node vertices without invalid output.
- `MyProject2.World.VoxelMesher.InvalidSampleRejected` — malformed density/material storage is rejected.
- `MyProject2.World.VoxelDensity.SnapshotRevisionAndGrid` — bordered snapshot shape, density changes after carve, revision advancement and refusal to reinterpret edited cells under another profile grid.

The runner also requires the seven existing Phase 1/2 world tests.

Run from Windows Git Bash/MSYS with Unreal Engine 5.8 installed:

```bash
bash Scripts/run_phase3_tests.sh
# Or set UE_ENGINE_ROOT if the engine is installed elsewhere:
UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_phase3_tests.sh
```

The script builds `MyProject2Editor` and runs `Automation RunTests MyProject2.World`. It must complete successfully before this phase can be called engine-verified.

## Still required before closing the implementation as validated

- Run the UE 5.8 editor target build and all 11 required world Automation tests.
- Open a test map with a Landscape and verify the generated surface visually.
- In PIE, carve and fill a sphere/box across a chunk boundary; confirm both neighbor meshes update, the player is blocked by the updated collision, and an out-of-date worker result is never published.
- Verify the scan finds the generated voxel surface as terrain support and marks it diggable only when the Phase 2 capability tags allow it.
- Inspect logs for invalid Landscape references, mesh/collision errors and pending worker jobs during map teardown.
- Profile CPU time, worker queue latency, collision cooking, memory and triangle counts on the intended LOW_MID hardware.

## Known limits

- No Unreal build, Automation run or PIE session was available while authoring this branch. Static source checks are not a substitute for them.
- Landscape integration reads loaded Landscape heightfields and uses physical-material mappings for coarse base material IDs. It does not inspect painted Landscape layer weight maps.
- Mesh generation is smooth in interpolation and normals, but density edits are currently binary sparse cell overrides. The surface silhouette is still bounded by the selected 25 cm voxel sampling resolution.
- LOD, adaptive resolution, persistence integration, gameplay scan-to-dig wiring, shoring/support-damage simulation and HIGH_END acceptance are later phases.
