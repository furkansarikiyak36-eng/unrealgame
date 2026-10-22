# Integration Status

**Current status: PARTIAL PORTABLE C++ PROTOTYPE — NOT COMPLETE — NOT UNREAL-INTEGRATED**

## Status by module
| Module | Current artifact | Remaining critical work |
|---|---|---|
| CoreContracts | Common IDs, vector/AABB, result/error types | Broader serialization and ID policies; compile/test evidence |
| RuntimeProfiles | LOW_MID/HIGH_END defaults and validation/session lock | More cross-profile constraints; tests |
| WorldScan | Provider contract plus initial ray/overlap wrappers | Provider implementation and engine adapter |
| WorldClassification | Deterministic evidence-precedence classifier | Landscape Builder sampling/provider and dirty-region incremental rebuild |
| TerrainVoxel | Sparse density overrides, bounded sphere/box/capsule edit, receipts, rollback and snapshot import/export | Materials/falloff policy, neighbor invalidation parity, broader stress tests |
| MeshCollision | CPU surface mesher, mesh validation, stale-revision gate; no publish queue or engine adapter | Full CPU mesher, mesh buffers/topology, bounded job queue, render/collision/nav adapters |
| PlacementSnap | Candidate filtering/ranking and commit revision check | Piece-specific providers, full collision adapter, placement/resource transaction |
| Excavation | Classification-authorized orchestration over density edit transaction | Capability/tool contracts, full event bus, integration tests |
| Shoring | Deterministic trench support-piece layout proposal | Surface anchoring/coverage solver, actor lifecycle, repair/carry/remove adapters |
| StructuralIntegrity | Deterministic DAG support assessment, damage, repair, collapse latch | More complete load distribution, replace/link lifecycle and stress tests |
| WorldEnvironment | Timestamped optional environment sample contract | Water/soil/weather providers and persistence ownership |
| Survival | Bounded exposure update and action effort costs | Canonical character-stat adapter and broader exposure rules |
| Persistence | Staged multi-participant restore transaction contract | World snapshot codec/migration, actual module participants and durable store |
| Validation | Nine portable test groups and CMake/CTest/CI config | Actual CI execution evidence and broader coverage |

## Evidence state
- Portable C++ source has been added for bounded algorithms above, including a CPU mesh extraction prototype and region-map overrides; this is not yet a complete game-mechanics implementation.
- The previous commit's GitHub Actions workflow completed successfully for seven portable test groups. The newly added classification-map and CPU-mesher tests are pending a fresh CI run.
- Existing `Source/MyProject2/` is still the Unreal implementation/reference area; migration parity has not been established.
- Unreal Engine 5.8 UBT/UHT, Automation, Editor/PIE and Unreal Insights have not been run as part of this module work.
- Schema v2/v3 stone persistence does not cover every carried/dragged tree/log/stone/rope/vehicle actor family.

## Status progression
`specified` → `prototype-partial` → `implemented` → `unit-tested` → `cross-module-verified` → `unreal-adapted` → `unreal-verified`.

Do not mark a module complete on the basis of a design document, source presence or a matching list of test names. Use the actual execution log as evidence.
