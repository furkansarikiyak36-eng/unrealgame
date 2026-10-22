# Low–Mid First Implementation Roadmap — Smooth Voxel, Scan, Placement, Shoring & Survival

**Date:** 2026-10-22  
**Target engine:** Unreal Engine 5.8 / C++  
**Status:** Architecture, cost model and implementation order. Not a claim that all systems are implemented or benchmarked.  
**Core policy:** One codebase, two selectable runtime profiles. Implement and verify the Low–Mid profile first; unlock the High-End profile only after profiling and validation.

---

## 1. Executive decision

We should not create two different game systems or two divergent branches for low/mid and high-end PCs. Build **one modular architecture**, expose two runtime profiles, and put performance-sensitive details behind interfaces/providers.

- **Profile A — Low–Mid First:** stable and conservative settings; sparse/edit-only voxel regions; short local queries; bounded chunk rebuild queue; limited collision work per frame.
- **Profile B — High-End:** higher near-player voxel resolution, more concurrent meshing, more active edit regions, finer collision and optional adaptive LOD/streaming optimizations.

The profiles share the same saved world format, gameplay rules, actor types and API contracts. A world saved in one profile should be loadable in the other; quality can change but logical world state must not change.

**Do not implement a second “digging engine.”** Smooth Voxel is both the terrain representation and the owner of excavation edits. Scan finds/analyses the target, Landscape Builder marks editable and static regions, Voxel Core changes terrain data, the meshing/collision adapter updates the physical world, and Placement/Shoring/Survival consume the new state.

## 2. What we mean by “cost”

No reliable absolute CPU-ms, RAM, or FPS figures can be promised before a working UE 5.8 build is profiled on target hardware. The tables below are **engineering estimates and acceptance targets**, not measured benchmark results.

Separate four costs:

1. **Build/authoring cost:** code and integration effort for a feature.
2. **Always-on runtime cost:** work charged even if the player is not digging.
3. **Action cost:** work caused by a scan, placement or excavation operation.
4. **Spike cost:** mesh/collision rebuild and save/load bursts that can stall a frame.

The architecture should minimize the always-on cost and queue the expensive action/spike work.

## 3. Two runtime profiles

### Profile A — LOW_MID (the first implementation target)

**Purpose:** Establish a playable excavation/building loop for lower and mid-range PC configurations before adding expensive quality features.

Initial configuration proposal (tunable after profiling):

- Target design gate: stable **30 FPS under a defined stress test**, with a 60 FPS baseline goal in light scenes. These are targets, not guarantees.
- Editable voxel regions only. Static mountains/rocks remain static meshes and are never converted to voxel data unless explicitly classified.
- Near-field density sample spacing starts at **25 cm** for the first performance prototype. This is an initial tuning value, not a final visual-quality commitment.
- Start with **8 m cubic voxel chunks**. At 25 cm spacing that is approximately 32 cells per axis, or about 32³ = 32,768 cells (boundary samples/metadata add overhead).
- Sparse edit deltas over a base density source. Do not store a dense high-resolution volume for the entire map.
- One simple, validated smooth meshing path first; do not build multiple meshing algorithms in parallel.
- Mesh generation works from immutable/plain data snapshots on worker tasks where safe; UObject/component mutation and publication happen on the game thread.
- Bounded mesh publish queue; batch collision refresh after geometry edits; prioritize chunks near the player and chunks the player can touch.
- Raycast for aim/interaction; local overlap only during placement; 3×3 or 5×5 sampling for surface checks. No global actor scans on Tick.
- Keep far terrain static or low-detail. No full-map voxelization and no per-frame Landscape Builder scan.
- Debris, loose soil simulation, water coupling and support rechecks use local/event-driven updates first.

**Value:** Less data, simpler profiling, faster iteration, fewer collision rebuild spikes. Main trade-off is lower geometric detail and fewer simultaneously editable regions.

### Profile B — HIGH_END (later, after Low–Mid acceptance)

**Purpose:** Increase fidelity and simultaneous activity without changing gameplay logic or save schema.

Optional upgrades:

- Reduce near-player spacing toward **12.5 cm** where a measurable visual/gameplay benefit exists.
- Use distance bands or adaptive detail, e.g. 12.5 cm near the active excavation, 25–50 cm away, and static meshes for uneditable areas.
- Run more independent meshing tasks in parallel, using measured worker-thread headroom rather than a fixed machine-independent promise.
- More active voxel chunks, finer collision near the player, larger edit throughput and richer support/soil responses.
- Partial chunk updates, mesh reuse/pools, async collision where supported and safe, more advanced surface/material blending.
- Optional LOD/transition scheme for different voxel resolutions. This requires explicit seam rules/tests; it is not merely a switch.
- More active debris, water flow, local material simulation and visual effects, all still governed by budgets.

**Value:** Higher quality and more simulation headroom. Trade-off: greater memory, mesh/collision work, test complexity and more demanding hardware.

### Relative cost example: same 8 m chunk

This is an order-of-magnitude illustration for dense sample counts, not a frame-time benchmark.

| Setting | 25 cm spacing | 12.5 cm spacing |
|---|---:|---:|
| Cells per axis (approx.) | 32 | 64 |
| Cells per chunk (approx.) | 32,768 | 262,144 |
| Relative volume samples | 1× | 8× |
| Approx. raw density storage if 4 bytes/sample, before overhead | 128 KiB | 1 MiB |
| Surface-triangle work for a comparable smooth surface | Baseline | Often grows about 4× with 2× finer linear resolution; actual result depends on topology |

This example excludes cell material IDs, metadata, chunk borders, generated vertices/indices, temporary work buffers, physics collision data and allocator overhead. **Actual RAM and CPU will be higher.** It explains why high resolution must be local and optional, not the default for the whole world.

---

## 4. Runtime budgets and performance acceptance

Unreal's total frame-time budget is 33.3 ms at 30 FPS, 16.67 ms at 60 FPS and 8.33 ms at 120 FPS. These budgets are shared by rendering, animation, AI, physics, gameplay, streaming and all other systems; voxel work does not own the whole frame.

Use the following as initial engineering gates, then replace them with measured target-machine results:

| Work item | LOW_MID initial gate | HIGH_END strategy |
|---|---|---|
| Normal Scan/interaction | Demand-driven; aim for <0.5 ms typical Game Thread cost when profiled | Same API; allow more detailed requests only when needed |
| Placement preview | Local queries; aim for <1.0 ms typical extra Game Thread cost | Cache and higher-resolution sampling when necessary |
| Density edit | Apply bounded local cell delta and enqueue dirty chunks | Batch edits; parallel preparation for independent chunks |
| Mesh generation | Worker-task generation from data snapshot; never rebuild the whole map | More parallel tasks, partial remesh, adaptive resolution/LOD |
| Mesh/collision publish | Bounded amount per frame; target <2 ms added Game Thread publication in routine cases | More queued throughput, async collision where supported |
| Static rock/mountain | Preserve static mesh and collision | Preserve static mesh; high-end rendering upgrades are separate |
| Landscape Builder | Editor/batch or explicit dirty-region jobs | Same; more parallel analysis only after determinism tests |
| Save/load | Delta-based storage; serialize off the hot interaction path when possible | Compression, asynchronous IO, versioned migrations |
| Water/soil/ecology | Local event-driven simulation | Wider active radius and more frequent simulation, budget controlled |

These are targets, not promises. A failed gate requires profiling and adjusting the implementation, not raising the limit without evidence.

**Frame spikes matter as much as average FPS.** For every excavation test, record median and worst-frame time, Game Thread time, mesher time, collision update time, dirty chunk count, active voxel chunk count, generated vertices/triangles, temporary memory, and save size/time. Use Unreal Insights and stat commands to profile these values on real target machines. (Epic profiling guidance: https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine)

## 5. One codebase: configuration and interface plan

A single runtime profile should be selected via a project setting/DataAsset, with editor overrides for test maps. Proposed core settings:

```cpp
enum class EWorldRuntimeProfile : uint8
{
    LowMid,
    HighEnd
};

struct FWorldRuntimeProfile
{
    EWorldRuntimeProfile Profile;
    float VoxelSampleSpacingCm;
    float ChunkSizeCm;
    int32 MaxMeshJobsInFlight;
    int32 MaxChunksPublishedPerFrame;
    int32 MaxSurfaceSamplesPerAxis;
    int32 MaxActiveEditableChunks;
    bool bUseAdaptiveVoxelLOD;
    bool bUseAsyncCollision;
};
```

This is an illustrative API contract, not compiled code. Values must be validated and clamped in the settings class.

Keep these interfaces stable even while replacing the implementation behind them:

- `IWorldClassificationSource`: reads Landscape Builder's voxel/static/unknown map.
- `IVoxelDensitySource`: samples base density plus excavation deltas.
- `IVoxelEditService`: applies an atomic carve/fill edit and returns dirty chunks/rollback metadata.
- `IVoxelSurfaceMesher`: converts a chunk density snapshot into vertices, indices, normals and material data.
- `IVoxelCollisionUpdater`: publishes/updates collision after mesh state is ready.
- `IWorldScanProvider`: offers demand-driven world queries and normalized analysis.
- `IPlacementSurfaceProvider`: offers anchor/snap candidates on log, plank, stone or voxel surfaces.
- `IStructureSupportAdapter`: validates a placed piece and responds to terrain invalidation.
- `IWorldPersistenceAdapter`: serializes versioned world deltas and placed actors.
- `ISurvivalRegionProvider`: reports local temperature, wetness, light and water state.

The first profile can use straightforward implementations; the High-End profile can later swap the mesher, collision policy, scheduling and detail strategy without rewriting gameplay. The shared contract must not expose assumptions that only one profile can satisfy.

## 6. Recommended project module layout

Use the current project modules where practical; do not create parallel canonical systems unnecessarily.

```text
Source/MyProject2/
  WorldCore/
    Classification/
      WorldClassificationTypes.h
      WorldClassificationMap.h/.cpp
      LandscapeBuilderSubsystem.h/.cpp
      ClassificationRules.h/.cpp
    Excavation/
      ExcavationVolumeSubsystem.h/.cpp      # existing sparse data core; audit and reuse
      VoxelEditService.h/.cpp
      VoxelChunkKey.h
    VoxelRuntime/
      VoxelRuntimeProfile.h/.cpp
      VoxelChunkMeshSubsystem.h/.cpp
      IVoxelSurfaceMesher.h
      MarchingCubesMesher.h/.cpp             # candidate Low–Mid implementation
      VoxelCollisionUpdater.h/.cpp
      VoxelDirtyChunkQueue.h/.cpp
    Persistence/
      WorldPersistenceAdapter.h/.cpp
  WorldScanSystem.h/.cpp                     # existing scan draft
  WorldSnapScanBridge.h/.cpp                  # existing bridge draft
  LogSnapSystem.h/.cpp                        # existing log snap draft
  Placement/
    PlacementValidationService.h/.cpp
    VoxelSurfaceAnchorProvider.h/.cpp
    PlankSnapProvider.h/.cpp
  Shoring/
    PlaceableShoringPiece.h/.cpp
    ShoringLayoutPlanner.h/.cpp
    ShoringSupportComponent.h/.cpp
    ExcavationShoringSubsystem.h/.cpp
  Survival/
    SurvivalExcavationAdapter.h/.cpp
    UndergroundExposureComponent.h/.cpp       # later optional system
  Tests/
    VoxelEditTests.cpp
    VoxelChunkSeamTests.cpp
    VoxelPlacementTests.cpp
    ShoringSupportTests.cpp
    VoxelPersistenceTests.cpp
    VoxelPerformanceScenarios.cpp
```

Some existing folders may differ in the current repository; treat this as the target logical layout. Before moving files, audit the current class ownership and avoid duplicate subsystems with overlapping responsibilities.

## 7. Implementation order: complete skeleton first, adapt step by step

The request is to establish all mechanical contracts early, then adapt and validate in order. Do this as **all interfaces/data contracts first, one executable vertical slice at a time**. Do not paste an enormous code drop that assumes every unfinished system is already connected.

### Phase 0 — Baseline and ownership audit

**Build:** inventory existing classes, source files, plugin dependencies, duplicate systems, main vs open feature branches. Select one canonical excavation data source and one canonical physical log/structure path.

**Deliverable:** dependency map, build baseline, accepted file/module plan and a test map.

**Exit gate:** clean UE 5.8 build and existing tests captured before new code is merged.

### Phase 1 — Contracts, settings and stubs for all systems

**Build:** runtime profile settings, versioned shared types, stable subsystem interfaces, capability tags, dirty-chunk events and failure/result codes for every planned block.

**Include stubs/contracts for:** classification, voxel edit, mesher, collision, scan, placement, shoring, structure support, survival environment query and persistence.

**Exit gate:** compileable declarations/modules, no duplicate owners, no gameplay yet depending on unvalidated placeholder success.

### Phase 2 — Low–Mid Scan + Classification

**Build:** merge/review Scanner and Snap draft work; build classification-map core; process only explicit/dirty regions; support static and unknown classes.

**Exit gate:** known objects are classified deterministically; static mountain collision remains unchanged; no whole-world scans on Tick.

### Phase 3 — Low–Mid Voxel data + first mesh/collision

**Build:** reuse sparse excavation core; implement the Low–Mid mesher and a small test area; generate smooth surface and collision; dirty chunk queue; boundary handling.

**Test:** carve a small pit, 2×2 m test shaft, 2×2 m tunnel crossing a chunk boundary, intersecting tunnel, rollback and save/reload.

**Exit gate:** visible mesh == collision == density state; player can enter/leave the dig; seam and reload tests pass.

### Phase 4 — Scan-to-dig and placement on voxel

**Build:** scanner hit → editable-area validation → voxel edit request; surface anchor candidates; preview/commit parity; local overlap; stale-surface invalidation.

**Test:** edit then immediately query the new surface; placement fails safely where collision/data are stale.

**Exit gate:** no ghost placements, no invisible blockers and no resource consumption on failed placement.

### Phase 5 — Shoring pieces and planner

**Build:** separate upright, lagging, strut, platform/ladder actors; planner for a configurable trench axis/width/depth and reference interval; ghost layout; commit; GUIDs and support anchors.

**Test:** place/reposition/remove pieces; reject conflicts; change voxel surface underneath and verify affected pieces are rechecked.

**Exit gate:** structure graph and placement system agree about which parts are connected and supported.

### Phase 6 — Support, damage and collapse

**Build:** one adapter between the current StructureLink/support graph and the new shoring pieces; deterministic support states; damage/replacement; explicit reactions to voxel/chunk invalidation.

**Exit gate:** a removed/broken support cannot remain silently “valid” in preview or saved state.

### Phase 7 — Survival integration

**Build:** action stamina costs, wetness/temperature/heat, light, underground access, water exposure. Keep oxygen/gas optional until the baseline loop is stable.

**Exit gate:** excavation and shoring change gameplay conditions; the state can be saved/reloaded without double-applying time or damage.

### Phase 8 — Persistence and complete integration

**Build:** versioned voxel edits, chunk density state, placed shoring actors, anchors/connections, actor damage and relevant player/world state in one load transaction.

**Exit gate:** restart the world and reproduce the same hole/tunnel, collision, supports, damage and player state.

### Phase 9 — Remaining existing mechanics

Integrate inventory/crafting, stone/stick systems, fire/raids, farming/animals, water/soil/material simulation and exploration against the stable contracts, one dependency group at a time.

**Rule:** no subsystem is considered integrated only because its source files compile. Require at least one end-to-end test from player action to authoritative world-state change.

### Phase 10 — High-End profile

Only after phases 0–8 pass under LOW_MID:

- enable finer near-field resolution and adaptive LOD;
- measure parallel meshing/collision throughput;
- implement and validate multi-resolution seams;
- raise active chunk limits based on memory and worst-frame data;
- widen water/ecology/simulation range if it stays within budget;
- compare both profiles on the same test scenes and save files.

**Exit gate:** High-End improves fidelity or throughput without changing gameplay validity, breaking saves or regressing the Low–Mid profile.

## 8. Test matrix for both profiles

Every profile uses identical functional acceptance tests; only performance/quality thresholds differ.

| Scenario | LOW_MID pass condition | HIGH_END pass condition |
|---|---|---|
| Scan static mountain | Classified static, stable collision | Same behavior with higher-quality query detail when requested |
| Small dig | Mesh/collision correct, bounded spike | Same functional result, higher detail allowed |
| Tunnel across chunk seam | No holes, ghost collision or gap | Same, including differing resolution transitions if LOD is enabled |
| Placement on cut surface | Preview and commit agree | Same |
| Shoring support removal | Dependent pieces re-evaluate | Same |
| Save/reload | Exact logical excavation and actor state restored | Same |
| Heavy dig | Queue stays bounded; no whole-world rebuild | Higher throughput; bounded memory and worst-frame |
| World change in static area | Static mesh stays unchanged | Same |
| Target hardware profile | Meets recorded project target on measured hardware | Meets separately recorded quality/performance target |

Record CPU/worker time, Game Thread time, render publication, collision build/update time, p95/p99 and worst-frame, active chunks, density samples, generated triangles, peak memory, save size and reload time. Do not accept a profile based on average FPS alone.

## 9. Initial implementation-cost assessment

The ratings below are **relative engineering effort**, not monetary quotes or completed work estimates.

| Work package | Relative engineering effort | Runtime cost tendency | Why |
|---|---|---|---|
| Scan/analyzer | Low–Medium | Low if demand-driven | Standard collision queries and classification |
| Landscape Builder | Medium | Mostly editor-time | Region sampling, rules, preview and map output |
| Sparse voxel edit data | Medium | Low–Medium | Local cells and edit transactions |
| Smooth mesher | High | High during edits | Surface extraction and mesh buffer construction |
| Collision adapter | High | High spike risk | Physics geometry must be rebuilt/updated correctly |
| Placement/Snap | Medium | Low–Medium | Candidate queries, overlap and validation |
| Shoring pieces/planner | Medium | Low–Medium initially | Actor placement, anchors and structure rules |
| Support/collapse integration | High | Medium; event spikes possible | Graph invalidation and dependent structure state |
| Basic survival adapter | Low–Medium | Low | Existing state values plus local environmental queries |
| Water/soil coupled to real excavation | High | Medium–High | Spatial continuity, chunk borders and persistence |
| Full save/load integration | High | Low while idle; burst on save/load | Multiple authoritative state owners and versioning |
| High-End adaptive resolution/LOD | Very High | Can increase throughput but adds overhead | Multi-resolution data, transition meshes, memory and seam tests |

**Largest technical risks:** dynamic mesh/collision rebuilds, chunk seams, inconsistent authoritative state across save/load, and a placed support whose voxel surface has changed. These should be solved before adding expensive high-end simulation.

## 10. Recommended first milestone

The first playable milestone should not attempt all 11 groups simultaneously. It should prove this complete vertical slice:

1. Landscape Builder marks one test patch as editable and a rock actor as static.
2. Player scans the ground and carves a smooth 2×2 m test trench.
3. Voxel mesh and collision update and match.
4. Player places one upright, one wall board and one strut using preview/snap.
5. Removing the strut updates support validity.
6. Save/reload restores the trench and pieces correctly.
7. LOW_MID profile passes the recorded performance test.

Once this works, add all remaining mechanics behind the same contracts and connect one by one. After LOW_MID is stable, High-End work is a profile/backend upgrade instead of another rewrite.

## 11. UE 5.8 implementation notes

- Unreal's `UDynamicMeshComponent` supports dynamic mesh changes and partial render-buffer updates, but Epic documents limitations including no Nanite, no LODs and no instanced rendering for Dynamic Mesh Components: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/GeometryFramework/UDynamicMeshComponent
- Geometry Scripting is marked Beta by Epic; use it for editor authoring/spikes if useful, but do not make the shipping runtime dependent on an unbenchmarked Blueprint geometry pipeline: https://dev.epicgames.com/documentation/en-us/unreal-engine/geometry-scripting-users-guide-in-unreal-engine
- Dynamic Mesh collision can be deferred until updates are needed: https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/DynamicMeshComponent/UpdateCollision
- Use Unreal Insights and stat commands to verify all budgets on target hardware: https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine

These engine documents identify available capabilities and limitations; they do not replace benchmarking the exact chunk/meshing design in this project.

---

## Final decision

**Build the full architecture contracts first, then implement LOW_MID in dependency order, then enable HIGH_END as a profile.** Landscape Builder decides where editable terrain is allowed. Smooth Voxel owns terrain data and excavation. Scan supplies current target/surface data. Placement and Shoring create separate physical actors on that terrain. Structure and Survival react to the resulting world state. Persistence stores the state owners together.

Do not promise CPU/RAM/FPS numbers until the working UE 5.8 prototype has been profiled. Use the sample density multipliers and relative work estimates to prioritize testing, not as measured performance claims.
