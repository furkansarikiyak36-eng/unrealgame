# MeshCollision — Specification

## Responsibility
Convert immutable TerrainVoxel snapshots into derived surface mesh/collision/navigation updates without becoming the terrain-state owner.

## Meshing responsibilities
- CPU Marching Tetrahedra is the initial option.
- Shared edge/node cache avoids duplicate vertices where topology permits.
- Produce positions, gradient normals, UVs and triangle material IDs.
- Handle exact isosurface values, border samples, winding and empty/full chunks deterministically.
- Validate every index, buffer length, finite vertex, triangle budget and topology invariant before publish.

## Async job contract
Each job records chunk coordinate + density source revision + profile identity + cancellation token. Worker jobs receive copied/immutable snapshot data and do not touch live world objects. A completed result is discarded when the current density revision has advanced or profile identity differs.

## Publish pipeline
1. Coalesce dirty chunk requests.
2. Acquire a bounded density snapshot with border.
3. Build mesh on a worker.
4. Validate topology/budgets.
5. Queue publish on the engine thread.
6. Publish render mesh.
7. Queue/update collision separately under collision budget.
8. Update navigation only where relevant and within its own budget.
9. Expose render/collision/nav revision stamps to invalidate scan and placement evidence.

## Budget policy
Concurrency, queue limits, mesh publishes per frame, collision updates per frame and active chunk counts come from RuntimeProfiles. Oversized queues must reject/coalesce/reprioritize requests explicitly; never grow without bound.

## Canonical-state rule
Mesh is derived from density. If mesh/collision update fails, density remains authoritative and the chunk stays dirty/needs-rebuild. Do not roll back a valid terrain edit solely because a derived job failed; expose rebuild debt and retry.

## Tests
Plane/corner cases, shared edges, exact iso value, index validity, material IDs, triangle cap, stale job rejection, queue saturation, border samples, neighbor seam refresh and render/collision revision mismatch.

## Status
Portable CPU surface-mesher source, mesh-buffer validation, a stale-revision gate and a bounded/coalescing dirty-chunk queue exist. Async worker scheduling and actual Unreal Dynamic Mesh, collision and navigation publishing remain unimplemented. The latest expanded CI run is pending.
