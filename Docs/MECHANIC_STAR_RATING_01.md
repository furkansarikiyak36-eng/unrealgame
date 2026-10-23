# Mechanic Star Rating 01 — Current Repository Maturity

**Audit date:** 23 October 2026  
**Scope:** main branch plus the unmerged modules/mechanics-pre-unreal prototype and referenced phase branches.  
**Important:** Stars measure current implementation/evidence maturity, not fun, visual quality, or proven parity with a commercial game.

## Rating scale

- ★☆☆☆☆ — mostly planned/specification; little executable behavior evidenced.
- ★★☆☆☆ — partial prototype/source exists, but integration and verification gaps dominate.
- ★★★☆☆ — substantial behavior exists and/or limited repeatable tests; important edge cases remain.
- ★★★★☆ — broad functional system and significant test evidence; some production gates remain.
- ★★★★★ — integrated, repeatedly verified in the target engine, performance-budgeted and robust across persistence/regression tests.

A historic passing log is useful evidence, but not a fresh build of current main. Portable C++ CI is not Unreal Engine 5.8 verification.

## A. New world-mechanics module track

| Mechanic | Best reference | Current stars | Why |
|---|---|---:|---|
| Core contracts, IDs, revisions and validation | Unreal Engine implementation discipline | ★★★☆☆ | Common types and validation contracts exist; broader serialization/identity policies and cross-module tests remain. |
| Runtime profiles / LOW_MID-HIGH_END | Target-hardware profiling | ★★★☆☆ | Profile defaults, range validation and grid lock exist; budgets need hardware measurement and cross-profile tests. |
| Raycast / shape sweep / overlap / surface sampling | No Man's Sky | ★★☆☆☆ | Portable query wrappers exist, but no concrete Unreal query provider is connected; query parity and stale-hit tests remain. |
| World classification and editable capability | No Man's Sky + 7 Days to Die | ★★☆☆☆ | Evidence precedence, region map and overrides exist; no UE Landscape/tag/physical-material provider, and incremental map updates have a scalability issue. |
| Terrain density / bounded dig-fill | 7 Days to Die + Enshrouded | ★★★☆☆ | Sparse overrides, sphere/box/capsule edits, revisions, operation IDs, receipts, dirty chunks and rollback exist in the portable prototype; UE end-to-end use remains unverified. |
| Smooth voxel meshing algorithm | Enshrouded / Teardown | ★★★☆☆ | CPU and separate UE-phase mesher drafts exist with interpolation, normals, material IDs and triangle caps; seam/iso-level/stress evidence is insufficient. |
| Render mesh + collision + navigation publication | Unreal runtime-mesh APIs; Teardown as behavior reference | ★★☆☆☆ | Revision gates/queues are partially prototyped, but UE render/collision/navigation publishers and PIE proof are missing. |
| Excavation authorization and transaction | 7 Days to Die + No Man's Sky | ★★☆☆☆ | Range, target and classification checks call a bounded edit; the raw hit source revision is not checked against the current world state and event wiring remains incomplete. |
| Generic snap candidate ranking | Sons of the Forest / Valheim | ★★☆☆☆ | Deterministic candidate filtering/ranking and commit revision checks exist; candidate providers, full-volume UE overlap, actor spawn and resource rollback are missing. |
| 20 cm plank grid | Project-specific rule; Sons of the Forest as interaction reference | ★★☆☆☆ | The requirement/contract exists, but it is not proven through an integrated candidate provider and target-engine acceptance tests. |
| Trench shoring planner and physical pieces | Custom system; partial construction references | ★★☆☆☆ | Deterministic layout and logical lifecycle exist; UE actor spawn, real anchors, coverage solving, physical links and resource-safe replacement are missing. |
| Structural support / damage / collapse | 7 Days to Die + Valheim | ★★☆☆☆ | Portable support DAG, damage/repair and collapse latch exist; load aggregation can overstate support when multiple dependents load the same supporter. No Chaos integration proof. |
| World-environment data contract | Timberborn / Oxygen Not Included | ★★☆☆☆ | Timestamped optional channels exist; real water/soil/weather/light providers and persistence ownership wiring are absent. |
| Survival exposure and effort cost | The Long Dark / Green Hell | ★★☆☆☆ | Bounded exposure and effort calculations exist; canonical character-stat adapter and broad time-step/exposure tests remain. |
| Persistence transaction / density snapshot | 7 Days to Die / Project Zomboid | ★★☆☆☆ | Density codec and staged-restore contract exist; base-density source/world identity, whole-world schema, actual participants, durable saves and restart/reload validation remain. |
| Test/validation evidence for the new chain | Our own reproducible acceptance tests | ★★☆☆☆ | Portable CMake/CTest CI passes nine grouped test suites, but this is not UE5.8 UBT/UHT, Automation, PIE, restart/reload or Unreal Insights evidence. |

## B. Existing broader mechanics families in the repository

These ratings are more favorable where main contains concrete behavior and historical automated checks. The test report dates to 14 October 2026 and does not prove a clean current-head build.

| Mechanic family | Best reference | Current stars | Why |
|---|---|---:|---|
| Tree harvesting, falling, log generation and splitting | Sons of the Forest | ★★★★☆ | Many physical behaviors are present; the recorded run covers tree falls, log splitting, carrying and derived pieces. Current-head UE verification is still missing. |
| Physical log cutting, carving, notches and shaping | Sons of the Forest | ★★★★☆ | Multiple cut/shape types, geometry changes and constraints are explicitly exercised in the recorded run; edge cases and fresh build evidence remain. |
| Physical assembly: posts, beams, floors, walls, ramps, braces and openings | Sons of the Forest / Valheim | ★★★★☆ | Substantial piece-role, support-path, orientation and structure-analysis coverage; the recorded 12 wood unit tests passed. Needs current-engine regression and integration evidence. |
| Wood snap / support path / structural joints | Sons of the Forest / Valheim | ★★★★☆ | Existing wood-specific snap and support tests exist, but are not the same as the newer generic PlacementSnap module. |
| Structural damage and collapse in existing wood system | 7 Days to Die / Valheim | ★★★☆☆ | Historic tests cover support removal, load failures, impacts and some collapse propagation. This does not prove general load distribution or new-module Chaos integration. |
| Material interaction family M1–M6 | Teardown / Valheim | ★★★☆☆ | Broad material-state family participates in fire/water/impact designs; no fresh comprehensive cross-material acceptance run was verified in this audit. |
| Campfire, fuel duration, ignition, burning and spread | Teardown and survival-game fire loops | ★★★☆☆ | The recorded run covers fuel scaling, re-ignition, contact spread, local heating, damage and burning limits. Complex geometry and performance still need testing. |
| Raid wave lifecycle and base damage | 7 Days to Die | ★★★☆☆ | Recorded checks cover starting/stopping a wave, spawning three enemies and structural damage. This does not prove full 7 Days to Die-grade path selection or adaptive raids. |
| Inventory and crafting integration | 7 Days to Die / Satisfactory | ★★★☆☆ | Several inventory, world-item, recipe/product and UI/bridge code families exist; end-to-end transaction, duplication and reload evidence was not fully verified here. |
| Stone gathering, handling and masonry | Valheim | ★★★☆☆ | Stone and masonry families exist; persistence does not cover every carried, dragged, falling or fractured state. |
| Crops, soil, planting and harvest | Farming Simulator / Project Zomboid | ★★★☆☆ | Multiple plant/harvest and soil/water families exist; long-running growth, environment integration and restart determinism need targeted tests. |
| Animals, feeding, production and reproduction | Farming Simulator | ★★☆☆☆ | Multiple stage families are listed, but a complete long-running feed → growth → reproduction → production → stock-ledger test was not verified. |
| World chunks/events, water/soil, decay and chemistry | Timberborn / Oxygen Not Included | ★★☆☆☆ | Broad code families exist, but connected behavior, numerical stability, update scheduling and performance budgets need an integrated test matrix. |
| Existing world SaveGame / full persistent world | 7 Days to Die / Project Zomboid | ★★☆☆☆ | An older save path exists, but failure-safe whole-world transactions and fresh-process restore were not evidenced; the new persistence module is not UE-integrated. |
| Test runner and current-head reliability | Our own validation gate | ★★☆☆☆ | Historic logs are useful, but Scripts/run_tests.sh has pipeline/exit-code risks, and no fresh current-head UE5.8 run was verified. |

## C. Composite scores

| Area | Rating | Interpretation |
|---|---:|---|
| Physical wood/log gameplay breadth | ★★★★☆ | Most mature and best evidenced family in the recorded report. |
| Existing fire / damage / basic raid loops | ★★★☆☆ | A meaningful baseline exists; advanced AI, integration and fresh runtime evidence remain. |
| Existing farming / animal / world simulation | ★★☆☆☆–★★★☆☆ | Many source families exist, but subsystem-specific long-run evidence is less clear. |
| New portable module architecture | ★★★★☆ | Ownership boundaries, revision rules and transaction design are clearly specified. |
| New portable module implementation | ★★★☆☆ | Real algorithms exist, but each module remains a partial prototype. |
| New-module automated portable evidence | ★★★☆☆ | Portable CI passes; its scope is narrow and it is not an Unreal test. |
| UE5.8 verification of the new vertical slice | ★☆☆☆☆ | No verified UBT/UHT + Automation/PIE + reload/performance gate for this full chain. |
| Overall production readiness | ★★☆☆☆ | Not production-ready yet; the main risk is integration correctness, not feature count. |

## D. What would raise the ratings?

### WorldScan: ★★☆☆☆ → ★★★★☆
Add a real UE5.8 provider; test actors, components, voxel surfaces, static geometry, ignored channels, result limits and stale observations; record a reproducible PIE run.

### Voxel and collision: ★★★☆☆ / ★★☆☆☆ → ★★★★☆
Run seam and iso-level stress tests; add asynchronous revision rejection; publish render mesh and collision through UE adapters; verify player movement against dig results and measure edit/collision budgets.

### PlacementSnap: ★★☆☆☆ → ★★★★☆
Add piece-specific candidate providers, full oriented-volume overlap, explicit sockets, configurable 0.20 m plank spacing, preview denial reasons and commit-time compensation for failed spawn/resource debit.

### Shoring and integrity: ★★☆☆☆ → ★★★★☆
Install actual UE pieces/anchors; compute coverage and link validity; aggregate dependent loads correctly; test damage, repair, terrain changes, removal and collapse in PIE.

### Persistence: ★★☆☆☆ → ★★★★☆
Persist world/base-source/profile identity, voxel deltas and piece states; preflight and stage every participant; prove failed load preserves the old world; test a fresh process restart and migrations.

## E. Primary evidence

- Main-branch historical mechanics test report: Docs/TEST_DURUMU.md, 14 October 2026.
- Module status and explicit Unreal integration gaps: Modules/INTEGRATION_STATUS.md.
- Mechanic inventory and required behavior: Modules/MECHANICS_CATALOG.md.
- Related existing mechanic families and boundaries: Modules/RELATED_MECHANICS.md.
- First reference comparison and vertical-slice acceptance plan: Docs/REFERENCE_GAME_COMPARISON_01_WORLD_SCAN_VOXEL.md.

**Bottom line:** our physical wood/log system is the strongest demonstrated family. The new module architecture has coherent design and real portable algorithms, but the scan → dig → mesh/collision → placement → save chain remains low maturity because target-engine integration and end-to-end evidence are missing. The next improvement should close and measure that chain rather than just add more systems.
