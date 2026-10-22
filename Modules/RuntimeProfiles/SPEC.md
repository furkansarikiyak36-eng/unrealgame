# RuntimeProfiles — Specification

## Responsibility
Select and validate the performance/grid policy for the world system. Individual modules must not invent competing profile defaults.

## Prototype defaults (not performance guarantees)
| Parameter | LOW_MID | HIGH_END |
|---|---:|---:|
| Density sample spacing | 0.25 m | 0.125 m |
| Chunk edge | 8 m | 8 m |
| Cells per axis | 32 | 64 |
| Approx. cells per chunk | 32³ | 64³ |
| Concurrent mesh job cap | 1 | 3 |
| Queue limit | 8 | 32 |
| Mesh publishes per frame | 1 | 2 |
| Collision updates per frame | 1 | 2 |
| Surface samples per axis | 5 | 9 |
| Target active editable chunks | 32 | 96 |
| Adaptive voxel LOD | Disabled | Future, disabled until verified |
| Async collision | Disabled until verified | Candidate, must be verified |

## Validation
- Positive finite spacing/chunk dimensions.
- Only supported cells-per-axis values.
- Job/queue/frame caps are nonzero and bounded.
- Grid dimensions and density spacing are consistent.
- Grid spacing/dimensions lock after first authoritative density snapshot.
- A profile change affecting storage requires explicit world migration/rebuild, not a silent live switch.

## Consumers
WorldScan uses query budgets; WorldClassification uses region-cell sizing; TerrainVoxel uses density grid dimensions; MeshCollision uses job/publish caps; Persistence stores profile/grid identity.

## Tests
Default values, invalid enums, queue bounds, profile switch before/after initialization and mismatch rejection on restore.

## Status
Specified; no portable RuntimeProfiles implementation is claimed here.
