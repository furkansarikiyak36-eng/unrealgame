# Integration Status

**Current status: PARTIAL PORTABLE C++ PROTOTYPE — NOT COMPLETE — NOT UNREAL-INTEGRATED**

## Status by module
| Module | Current artifact | Remaining critical work |
|---|---|---|
| CoreContracts | Common IDs, vector/AABB, result/error types | Broader serialization and ID policies; compile/test evidence |
| RuntimeProfiles | LOW_MID/HIGH_END defaults and validation/session lock | More cross-profile constraints; tests |
| WorldScan | Provider contract plus initial ray/overlap wrappers | Real world-query provider, actual engine adapter and broad query tests |
| WorldClassification | Deterministic evidence-precedence classifier | Unreal actor/component/Landscape sampling provider and richer provenance/cache invalidation |
| TerrainVoxel | Sparse density overrides, bounded sphere/box/capsule edit, receipts, rollback and snapshot import/export | Materials/falloff policy, neighbor invalidation parity, broader stress tests |
| MeshCollision | CPU surface mesher, mesh validation, stale-revision gate and bounded dirty-chunk queue; async scheduling and engine publisher remain | Full CPU mesher, mesh buffers/topology, bounded job queue, render/collision/nav adapters |
| PlacementSnap | Candidate filtering/ranking and commit revision check | Piece-specific providers, full collision adapter, placement/resource transaction |
| Excavation | Classification-authorized orchestration over density edit transaction | Capability/tool contracts, full event bus, integration tests |
| Shoring | Deterministic trench layout proposal plus logical assembly lifecycle (validate/install/revalidate/damage/repair/remove) | Unreal actor spawn/anchors/coverage, carry/replace, resource rollback adapters |
| StructuralIntegrity | Deterministic DAG support assessment, damage, repair, collapse latch | More complete load distribution, replace/link lifecycle and stress tests |
| WorldEnvironment | Timestamped sample contract plus bounded portable water/soil/heat/fire simulation, fixed-step backlog and water-balance ledger | Unreal Landscape/weather/query adapter, chunk-boundary exchange, region streaming/scheduler, durable environment persistence and gameplay tuning |
| Survival | Bounded exposure update and action effort costs | Canonical character-stat adapter and broader exposure rules |
| Persistence | Staged multi-participant restore transaction contract | Portable density-snapshot codec now exists; whole-world schema/migration, module participants, durable storage and actor/player adapters remain |
| Validation | Eleven portable test groups defined; Phase 4 hydrology, backlog, soil ledger, fire and survival integration regressions added | Phase 4 branch CI must run before claiming the new tests pass; broader property/stress/performance coverage and Unreal verification remain open |

## Evidence state
- Portable C++ source covers bounded algorithms, including a CPU mesher, classification map/dirty-cell builder, shoring lifecycle and density snapshot codec. This remains a partial mechanics prototype, not a complete game.
- Historical GitHub Actions for commit `2a4852ce43622ed54485c495d4dbcb6b2eb41138` completed successfully: CMake configure/build and CTest passed with nine portable test groups. That is not UE validation.
- Phase 4 added two additional portable regression groups. Its own CMake/build/CTest run must pass before this branch can be marked portable-test verified.
- Existing `Source/MyProject2/` is still the Unreal implementation/reference area; migration parity has not been established.
- Unreal Engine 5.8 UBT/UHT, Automation, Editor/PIE and Unreal Insights have not been run as part of this module work.
- Schema v2/v3 stone persistence does not cover every carried/dragged tree/log/stone/rope/vehicle actor family.

## Status progression
`specified` → `prototype-partial` → `implemented` → `unit-tested` → `cross-module-verified` → `unreal-adapted` → `unreal-verified`.

Do not mark a module complete on the basis of a design document, source presence or a matching list of test names. Use the actual execution log as evidence.
