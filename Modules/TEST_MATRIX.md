# Module Acceptance and Test Matrix

A row is PASS only after tests are executed and output/environment are recorded. Test registration/name coverage is not a passing run.

| Module | Unit/contract tests | Cross-module tests | External engine gate |
|---|---|---|---|
| CoreContracts | Invalid values, stable IDs, finite vectors, revision semantics, unit conversion | Every request/snapshot uses common validation | Portable compiler/CI not established |
| RuntimeProfiles | Defaults, enum/range errors, budget bounds, grid lock | One profile is consistent across scan/terrain/meshing | Measure UE 5.8 cost |
| WorldScan | Hit/miss, filters, range, zero direction, cap, stale result | Scan evidence cannot bypass dig authorization | Real trace/collision behavior |
| WorldClassification | Precedence, AABB edges, Unknown, revision | Static/Unknown target denied by dig policy | Landscape/tag/layer data in Editor |
| TerrainVoxel | Base sample, sparse override, shape bounds, negative chunks, revision, rollback | Dig→snapshot→reload reproduces samples | Landscape source and large-world scale |
| MeshCollision | Topology, finite vertices, triangle cap, shared edges, stale job, seam | Dig result agrees between visible surface and collision | Dynamic Mesh/collision/navigation/PIE |
| PlacementSnap | Normal/axis/distance, 20 cm grid, overlap, stable tie, stale revision | Preview equals commit only while evidence is current | Physics overlap and ghost preview |
| Excavation | Tool/capability/range, duplicate op, no-op, rollback, event/dirtied chunks | Scan→classify→dig | Character input/animation |
| Shoring | Station endpoints/spacing, counts, dimensions, links, missing pieces, rollback | Dig→plan→commit→edit invalidation | Spawn/fit/physics in UE |
| StructuralIntegrity | Duplicate/dangling IDs, cycle policy, capacity boundaries, damage, repair, support loss, collapse latch | Remove support→reassessment/event | Chaos/physics consequence |
| WorldEnvironment | Missing/stale channels, finite input, source revision, time step | Environment→survival and persistence | Weather/water/light providers |
| Survival | Bounded time step, exposure recovery, missing inputs, effort costs, snapshot | Dig/carry/repair→canonical stamina | Character component integration |
| Persistence | Schema migration, refs, duplicate IDs, staged failure, rollback, round-trip | Full vertical slice reload/revalidation | Real slot/world restart/PIE |
| Validation | Runner exit behavior, report metadata, deterministic scenario | Entire workflow above | UBT/UHT, Automation, PIE, Insights |

## Performance scenarios
- LOW_MID and HIGH_END configured chunk/edit/queue limits.
- One excavation affecting multiple chunks and neighbors.
- Repeated same-region edits coalesced into bounded dirty queue.
- Old async mesh job completing after a newer edit.
- Save/load time and payload size for increasing sparse override/piece counts.
- Measure 10k/100k density overrides and 100+ shoring pieces on target hardware; do not promise performance before measurement.
