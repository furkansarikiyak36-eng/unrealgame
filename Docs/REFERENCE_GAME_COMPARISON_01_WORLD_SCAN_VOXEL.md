# Reference Game Comparison 01 — World Scan, Voxel Terrain, Digging, Mesh/Collision and Snap

**Audit date:** 23 October 2026  
**Scope:** source review of main plus the unmerged mechanics-module and phase branches.  
**Purpose:** reverse-design comparison and an executable implementation/test plan. This is not a claim of feature parity with the reference games.

## 1. Executive decision

Do not copy one game as a whole. Use each reference for the problem it demonstrates best:

- **7 Days to Die:** editable voxel world, excavation/build loop, structural rules and how terrain/structures affect base defence.
- **Enshrouded:** terrain shaping and voxel-building freedom. The official site and Steam listing describe a fully terraformable world and voxel-based construction. Steam lists its 1.0 release as 15 October 2026.
- **No Man's Sky:** explicit scanning interaction, semantic discovery information, terrain-manipulator modes and variable edit shapes.
- **Teardown:** consequences of a destructible voxel world, including physics, fire, smoke, water, debris and vehicles. Its FAQ says it uses a custom voxel-based engine built specifically for the game, so compare behavior, not assume its engine design can be ported to Unreal.
- **Sons of the Forest:** piece-based placement prompts, log length/orientation choices and readable placement candidates.
- **Unreal Engine 5.8 docs:** the technical implementation reference for runtime mesh and collision updates.

The target chain is:

**Aim/scan → raw hit → classify target → preview bounded edit → commit density change → mark dirty chunks → build from immutable density snapshot → reject stale mesh result → publish render mesh → update collision → re-query world → place piece on valid snap candidate.**

Having one C++ class for each stage does not mean the full chain works. Engine adapters, cross-system tests, runtime evidence and performance measurement are separate gates.

## 2. Reference games: what to take and what not to assume

### 2.1 7 Days to Die — voxel world and structural rules

The game wiki describes a fully destroyable/buildable voxel world. Its structural-integrity documentation separates block mass, vertical support and horizontal support; placing or removing blocks triggers stability checks, and connected unsupported blocks can collapse. Placement preview communicates expected stability and obstruction.

**Take:** terrain edits must change authoritative world state; nearby structures must be invalidated; failed placement should be visible before commit; digging should affect what the player can build and defend.

**Do not assume:** block-grid rules map directly to smooth density voxels; a block support algorithm maps directly to a physical log/socket system; or the reference game's voxel resolution and performance settings are optimal for our world.

### 2.2 Enshrouded — terrain shaping and construction freedom

Official material presents a fully terraformable land with voxel-based building, including carving into hillsides and reshaping terrain around construction.

**Take:** terrain tool modes, sizes and player-facing preview matter; terrain and building must work together; newly created holes, slopes and surfaces must remain consistent with collision and placement.

**Do not assume:** visual similarity means we share its density representation, meshing algorithm or collision architecture; or that it is our structural-integrity reference.

### 2.3 No Man's Sky — scanning as a deliberate interaction

Hello Games' official updates describe the Analysis Visor as a deliberate scan interaction that reveals additional information about flora, fauna and minerals. Terrain-manipulator updates describe adding/removing terrain and selecting materials, shapes and sizes. The Beyond update documents aiming at a target and holding a button to scan.

**Take:** a raycast is only a raw observation. It should not decide what the target means or whether it can be changed. Keep separate raw facts (target ID, point, normal, distance, source revision) and semantic facts (EditableVoxel, StaticGeometry, Water, Transition, Unknown). Make the scanner report both capability and the reason an action is denied.

**Do not assume:** a visible/highlighted target is safe to mutate; every scanner needs hundreds of rays every frame; or a scan remains valid after the world changes.

### 2.4 Teardown — destructible world and downstream consequences

The official site and Steam listing describe a destructible voxel environment and physics, fire, smoke, water, debris and vehicles. Its FAQ explicitly identifies a custom voxel-based physics/graphics engine.

**Take:** test beyond surface appearance. Render geometry, collision, navigation and debris are separate downstream products. Chunk boundaries, edit bursts, stale jobs and resource budgets are first-class problems.

**Do not assume:** Unreal Engine automatically gives Teardown-like destruction when we use a runtime mesh component; or that a custom-engine technique is transferable without measurement.

### 2.5 Sons of the Forest — placement prompts and physical pieces

Community construction documentation describes log length variants, horizontal/vertical/diagonal placement, prompts around nearby logs, and deliberate cutting actions. Use these as interaction-design observations, not authoritative engine implementation details.

**Take:** generate placement candidates for the intended piece and its nearby sockets/surface. Provide a deterministic transform preview and clear invalidity reasons. Log sockets, free surface placement and a 20 cm plank grid are different candidate providers.

**Do not assume:** a valid pivot means the entire oriented object fits; a candidate remains valid after the surface moves; or a universal grid is right for every piece.

## 3. Repository audit: present versus missing

### 3.1 Main branch baseline

- **Docs/TEST_DURUMU.md** records 101 passed / 0 failed mechanics checks and 12 passed wood unit tests from 14 October 2026. These are results of that recorded run, not proof of a fresh clean build of the current main HEAD.
- The repository has existing families for wood/log processing, material interactions, fire, soil/water, world simulation and raids. Their presence does not prove that the new terrain-to-collision vertical slice passes.
- Phase implementation work is on separate open/draft branches. Source on a phase branch is not evidence that the implementation is merged into main.

### 3.2 Mechanics-module branch

Branch: **modules/mechanics-pre-unreal** (PR #19, draft at audit time). The manifest describes it as a partial portable prototype and explicitly says it is not Unreal-integrated.

| System | Code located | Current evidence and remaining gap |
|---|---|---|
| WorldScan | Modules/WorldScan/include/world_mechanics/world_scan.hpp | Validated wrappers for ray, shape sweep, overlap and surface sampling. Provider defaults return Unsupported. No concrete UE world-query provider. |
| WorldClassification | Modules/WorldClassification/src/classification_map.cpp and Modules/WorldClassification/include/world_mechanics/landscape_builder.hpp | Evidence precedence, region grid, manual AABB overrides and dirty-cell builder contract. No UE Landscape/tag/physical-material probe implementation. |
| TerrainVoxel | Modules/TerrainVoxel/src/terrain_voxel.cpp | Sparse density overrides, base source, bounded sphere/box/capsule edits, revision guard, duplicate-operation protection, rollback receipts, dirty chunks and snapshot import/export. |
| MeshCollision | Modules/MeshCollision/src/cpu_surface_mesher.cpp | Portable CPU surface mesher, edge cache, normals/UVs/material IDs, triangle budget and mesh validation. No UE render/collision publisher. |
| PlacementSnap | Modules/PlacementSnap/include/world_mechanics/placement_snap.hpp | Ranks supplied candidates by socket, distance and stable ID, filters normal/axis/region/overlap/revisions, validates again at commit. Does not generate candidates, execute UE overlap queries or spawn actors. |
| Excavation | Modules/Excavation/src/excavation.cpp | Checks target ID, distance, edit offset, classification revision and density edit result. It does not validate that the raw hit's source revision is still current. |

### 3.3 Separate Unreal phase prototype

On branch **feature/phase-03-lowmid-smooth-voxel**, Source/MyProject2/WorldCore/WorldVoxelMesher.cpp contains an Unreal C++ Marching Tetrahedra mesher, and ExcavationVolumeSubsystem.cpp contains a runtime density-override/edit subsystem. These are engine-facing drafts, not portable specifications only. But UBT/UHT, Automation and PIE validation have not been established in the phase evidence. They are not merged into main and must not be represented as main-branch completion.

## 4. Deep comparison by mechanic

### 4.1 WorldScan — query facts, not gameplay decisions

**Required contract**
1. Query request: origin, normalized direction or swept shape, range, ignored IDs, trace policy/channel, result cap and expected revision where relevant.
2. Raw result: hit/miss, stable target ID, point, normal, distance, source revision or timestamp.
3. Validate finite values, ranges, directions, result caps and deterministic sorting.
4. Pass raw evidence to WorldClassification; do not embed dig permission in raycast code.
5. Treat miss, unsupported provider, stale observation and corrupt provider response as distinct results.

**Finding:** the portable header implements wrappers for ray, shape sweep, overlap and surface sampling. Provider functions are integration seams with default Unsupported results, not Unreal traces.

**Fixes and tests**
- Implement UE5.8 trace/sweep/overlap adapters with stable identity mapping, ignored actors/components and explicit collision channels.
- Add the required bounds/physics observation contract if gameplay consumes it. The current provider has no dedicated bounds/physics-observation wrapper.
- Make query budgets consistent. Overlap validates its bounds and result count but does not call the runtime profile validator at the top, unlike Ray/ShapeSweep/SampleSurface.
- Use RawHit.sourceRevision. Excavation currently consumes the observation but does not compare its source revision with the current target/surface revision or enforce observation age.
- Test hit/miss, zero direction, invalid range, ignored target, static-mesh hit, voxel hit, capped overlap, invalid normals, stale hit and deterministic result ordering.

**Acceptance rule:** a raw hit cannot authorize digging or placement. Authorization requires a current observation, classification, capability and operation policy.

### 4.2 WorldClassification — evidence precedence; Unknown is not editable

**Required classes:** EditableVoxel, StaticGeometry, Transition, Water and Unknown.

**Required evidence policy**
1. Authored manual override with a stable ID.
2. Trusted actor/component tags, Landscape layers or physical-material mappings.
3. Existing cached evidence with provenance and revision.
4. Bounded geometry inference as a lower-confidence fallback.
5. Unknown when evidence is insufficient or contradictory.

A classification result should explain why it won, not merely return an enum.

**Finding:** ClassificationMap supports evidence, manual AABB overrides and a dirty-cell builder contract. UpsertCells copies the entire current map for every small incremental update (auto staged = cells_) and uses maxCellCount × 8 as a total storage cap. That makes update cost scale with total map size and couples a per-batch argument to lifetime map capacity.

**Fixes and tests**
- Split max cells per batch, total map cells and memory budget.
- Replace full-map copying for local updates with a bounded transaction over touched keys.
- Preserve evidence provenance, deterministic priority and monotonic revisions.
- Implement UE probes for actor/component tags, Landscape layer data and physical materials.
- Test conflict resolution, Unknown default, static-mesh denial, editable Landscape allowance, override add/remove, AABB boundaries, duplicate dirty keys, rollback on failed batch and a small update to a large map.

**Acceptance rule:** Unknown and StaticGeometry are denied by default. Failed classification may not silently mean “editable.”

### 4.3 TerrainVoxel and Excavation — authoritative density transaction

**Required transaction**
1. Validate tool, target capability, range, shape, profile and expected revisions.
2. Preflight the affected-cell bound against the budget before mutation.
3. Stage all changes, then commit atomically.
4. Increment authoritative density revision once for a non-empty edit.
5. Return a receipt with prior data, changed cells/chunks and old/new revisions.
6. Send a typed dirty-region event to dependent systems.
7. Permit rollback only while no later edit invalidates the receipt.

**Finding:** portable TerrainVoxel implements sparse overrides, bounded sphere/box/vertical-capsule edits, operation IDs, expected-revision checks, edit budgets, dirty-chunk calculation and rollback receipts. Excavation validates target ID, hit distance, edit offset and classification revision before applying the edit.

**Critical gap:** the observation's source revision is not compared with current target/surface state. That leaves a stale-scan window.

**Persistence gap:** the density snapshot stores grid/revision/overrides/operation IDs, but a sparse delta snapshot also needs base-density source and world/map/profile identity. Loading the same overrides against a different base source can reconstruct different terrain.

**Fixes and tests**
- Test all edit shapes, invalid bounds, oversized edits, negative coordinates and chunk edges.
- Prove duplicate operation IDs and stale expected revisions fail without partial mutation.
- Prove rollback succeeds immediately but refuses to cross a later edit.
- Verify dirty neighbors at chunk faces, edges and corners.
- Verify dig/fill and snapshot round-trips on known samples.
- Reject snapshots with mismatched base-source/world identity.
- Stress repeated edits and concurrent/out-of-order job requests.

### 4.4 MeshCollision — mesh is derived data, not truth

**Engine reference:** UE 5.8 ProceduralMeshComponent supports custom triangle geometry, but its API documentation labels the feature experimental. UpdateMeshSection is faster than recreating a section but cannot change topology. DynamicMeshComponent exposes deferred collision updates and async cooking options. Therefore component choice should be decided by an actual UE prototype and benchmark, not guesswork.

**Required pipeline**
1. Capture immutable density/material snapshot for chunk N at revision R.
2. Build mesh off-thread from that snapshot.
3. Validate finite attributes, index ranges, material-buffer lengths, topology invariants and triangle budget.
4. Before publishing, compare R with the latest density revision for that chunk.
5. Discard stale mesh if revisions differ.
6. Publish render mesh; schedule collision separately under explicit budgets.
7. Track density, render and collision revisions independently, and measure collision lag.
8. Invalidate affected neighboring chunks from border samples.
9. Update navigation only where necessary; do not synchronously rebuild the whole nav world for every local edit.

**Finding:** the portable mesher uses six tetrahedra per cell, edge caching, interpolated normals, UVs, triangle material IDs, a hard triangle budget and output validation. The UE phase branch has a Marching Tetrahedra mesher too. Neither proves collision and movement remain synchronized in PIE.

**Risks/tests**
- **Chunk seams:** identical shared border samples must produce continuous surface at faces, edges and corners. A local per-chunk edge cache does not, alone, stitch two chunk meshes.
- **Iso-level equality:** samples exactly at iso threshold can create coincident vertices/degenerate triangles; test and define deterministic handling.
- **Material policy:** compare portable and UE material-selection rules when multiple solid materials meet, avoiding silent divergence.
- **Collision freshness:** after digging, player must not stand on removed terrain, fall through retained terrain or hit an invisible old wall.
- **Stale work ordering:** finish a revision-R job after an R+1 edit and prove R cannot publish as current.
- **Bounded cost:** coalesce dirty work and cap queue bytes, triangles, task size and per-frame publishing/collision updates.
- **Stress shapes:** cave opening, thin wall, tunnel across chunk boundary, all-solid/all-empty, checkerboard density and repeated edit while moving.

**Acceptance rule:** density remains authoritative if mesh build fails; mark explicit rebuild debt. Never publish an old render/collision product as current.

### 4.5 PlacementSnap — generate candidates, then commit

**Required pipeline**
1. Select a candidate provider by piece family: free surface, explicit socket, log joint or fixed-spacing plank grid.
2. Generate transforms with stable IDs and source surface/socket identity.
3. Filter by range, capability, normal, axis, part compatibility, revisions and complete collision volume.
4. Rank deterministically: valid explicit socket where appropriate, then distance, then stable ID.
5. Show ghost transform and explicit invalidity reason.
6. Recheck revision, full-volume overlap and resource state at confirmation.
7. Commit spawn/link/resource debit atomically; compensate on failure.

**Finding:** PlacementSnapResolver ranks a supplied candidate list by explicit socket, distance and stable ID, and checks normal/axis, region, overlap and revisions. It does not generate piece-specific candidates or execute Unreal collision checks/actor spawning.

**Fixes and tests**
- Implement separate candidate providers; keep the resolver policy-only.
- Make plank spacing configurable. Test 0.20 m spacing at different origins; do not force this grid on terrain tools or log sockets.
- Test full oriented bounds, not only pivot position or a pre-populated overlap boolean.
- Invalidate previews after terrain/classification changes.
- Test rotated pieces, steep slopes, overhead obstruction, actor overlap, stale preview and failed spawn/resource debit.

## 5. Reference matrix

“Strong” means relevant as a design reference, not proof of parity.

| Target requirement | 7 Days to Die | Enshrouded | No Man's Sky | Teardown | Sons of the Forest | Our module evidence |
|---|---|---|---|---|---|---|
| Editable terrain as persistent world change | Strong | Strong | Strong | Strong | Not primary | Partial density edit; UE flow absent |
| Smooth terrain shaping/build freedom | Partial; block-oriented | Strong | Partial | Strong destructive behavior; different engine | Not primary | Density + mesher prototypes; publish unverified |
| Scan and semantic information | Partial | Not primary | Strong | Not primary | Partial | Wrappers/classifier exist; UE provider absent |
| Capability-based dig authorization | Useful comparison | Useful comparison | Partial | Not primary | Not primary | Partial; source revision stale check missing |
| Chunk/job/collision correctness | Behavior reference | Behavior reference | Behavior reference | Strong behavior reference, engine not transferable | Not primary | CPU mesher/revision structures; engine publisher absent |
| Socket/orientation placement | Partial | Partial | Base placement only | Different goal | Strong | Resolver exists; candidate providers/UE overlap absent |
| Configurable 20 cm plank grid | Not direct | Not direct | Not direct | Not direct | Conceptual placement reference | Must be wired and tested |
| Current-engine verification | We need our own | We need our own | We need our own | We need our own | We need our own | Portable CI only; UE5.8 tests absent |

## 6. Prioritized work plan

### P0 — correctness before adding features
1. Add UE5.8 WorldScan provider with stable IDs, channels, ignored actors, range checks and revision capture.
2. Fix classification map update cost and separate batch/total-map budgets.
3. Reject excavation when observed source revision is stale.
4. Add base-density source identity, world/map ID, runtime-profile fingerprint, origin/grid/chunk identity and schema to persistence metadata.
5. Define density snapshot → async mesher → revision gate → render publisher → collision publisher; track collision debt.
6. Add piece-specific snap providers, full-volume UE overlap and configurable 20 cm plank spacing.
7. Harden the test runner so a failed build/test cannot be hidden by grep/pipeline status.

### P1 — integration and stress evidence
8. Run mesher seam and exact-iso tests, including boundary cases.
9. Run PIE vertical slice: scan → classify → dig → render update → collision update → walk through hole/stand on ground → snap plank → change surface → reject stale placement.
10. Save, restart the session/process, reload, then rerun scan/collision/snap checks.
11. Measure LOW_MID and HIGH_END profiles: CPU time, memory, edit cells, mesh triangles, async backlog, collision update time and worst-frame cost. Set budgets from measured results rather than promising FPS upfront.

### P2 — scale and polish
12. Add preview and denial reasons for scan/dig/place.
13. Add telemetry/debug capture for render-collision mismatches and revision disagreements.
14. Stress 10k and 100k sparse overrides, multi-chunk edit bursts and large candidate lists on target hardware.

## 7. Acceptance test: first vertical-slice gate

Use a test map with editable voxel terrain, a static-mesh boulder and an unknown surface.

1. Scan the boulder: report StaticGeometry; dig is rejected; density revision does not change.
2. Scan the unknown surface: fail closed; no dig occurs.
3. Scan editable terrain: report stable target ID, point, normal and source revision; classification includes provenance.
4. Preview a bounded dig sphere; invalid radius or over-budget edit is rejected before mutation.
5. Commit dig: unique operation ID, exactly one density revision increment, receipt lists changed cells and dirty chunks.
6. Build mesh for revision R; commit a second edit at R+1 before the first job finishes. The old result must be discarded.
7. Publish current mesh and collision. The player passes through new void but still collides with retained ground.
8. Preview a plank on a configurable 0.20 m grid using full oriented bounds. Edit the underlying surface before confirmation; stale candidate must be rejected.
9. Save, restart/reload, then repeat scan, collision and snap checks.
10. Record engine/build, profile, map and grid dimensions, timings, triangle count, collision revision and executed pass/fail output.

**Gate criteria**
- No unauthorized or stale dig commits.
- Failed preflight leaves no partial density mutation.
- Stale mesh result is never published as current.
- Render/collision revisions converge within an explicit budget or emit measurable rebuild debt.
- Boundary tests show no chunk holes/discontinuities in the target shapes.
- Placement fails safely if revision, overlap or resource state changes.
- Reload reproduces authoritative density and relevant placement behavior.
- Executed logs are the evidence; source presence or registered test names alone do not count.

## 8. Sources and limitations

Game references:
- 7 Days to Die: https://7daystodie.wiki.gg/wiki/7_Days_to_Die and https://7daystodie.wiki.gg/wiki/Structural_Integrity
- Enshrouded official page and Steam listing: https://www.keengames.com/enshrouded and https://store.steampowered.com/app/1203620/Enshrouded/
- No Man's Sky official updates: https://www.nomanssky.com/atlas-rises-update/ and https://www.nomanssky.com/beyond-update/
- Teardown official site, FAQ and Steam listing: https://teardowngame.com/ and https://teardowngame.com/faq.html and https://store.steampowered.com/app/1167630/TeardownGame/
- Sons of the Forest construction behavior reference: https://sonsoftheforest.fandom.com/wiki/Custom_Building
- Unreal Engine 5.8: https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/ProceduralMeshComponent and https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/GeometryFramework/UDynamicMeshComponent

Limits:
- Public game descriptions show behavior, not complete proprietary implementation.
- Community construction documentation may lag patches; use it as behavior observation.
- We have not benchmarked the reference games' algorithms or performance on our hardware.
- This comparison claims neither UE integration nor feature parity; those require the acceptance tests above.

## 9. Decision

Implement in this order: **real scan provider and trusted classification → revision-safe density transaction → snapshot-driven mesh/collision publication → placement candidate providers → full PIE and restart/reload validation.**

Do not broaden feature count until this chain passes. The highest-risk gap is not another tool shape; it is ensuring observation, authoritative edit, derived mesh/collision, placement preview and persisted state all refer to the same current world revision.
