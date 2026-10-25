# CoreContracts — Specification

## Responsibility
Define the shared language all modules use: common identifiers, primitive values, result/error contracts, revision semantics and invariants. It owns no gameplay mechanics.

## Required domain types
- `EntityId`, `RegionId`, `ChunkId`, `OperationId`, `PlanId`, `SnapshotId`: stable opaque IDs, not actor pointers.
- `Vec3`, `Normal3`, `Transform`, `Aabb`: finite values with explicit coordinate convention.
- `Revision`: monotonic per authoritative source; zero means unknown/uninitialized.
- `ModuleResult<T>`: success or typed error: InvalidArgument, OutOfRange, UnsupportedState, StaleRevision, Conflict, BudgetExceeded, MissingDependency, CorruptData, InternalFailure.
- `ValidationIssue`: stable code, severity, field/path and human-readable explanation.
- `WorldContext`: world/map identity, profile identity and current time/tick context.

## Rules
1. Neutral domain APIs use meters unless a field explicitly says otherwise. Unreal adapters convert centimeters at one boundary only.
2. Reject NaN/Inf and invalid extents before calculations/serialization.
3. IDs are not pointers and are not reused while live records can reference them.
4. Results carry enough context to diagnose failure.
5. Cross-module payloads are value records; no private-layout peeking.
6. Mutating operations carry an operation ID and expected source revision where retry/staleness matters.
7. Serialization uses stable fields and schema versions, never raw object memory.

## Boundary ports
- `IWorldQueryProvider` is implemented by WorldScan adapters.
- `IBaseDensitySource` is implemented by TerrainVoxel providers.
- `IMeshCollisionPublisher` is implemented by MeshCollision adapters.
- `IPlacementQueryProvider` is used by PlacementSnap.
- `IWorldSnapshotParticipant` is coordinated by Persistence.
These are interfaces over common value types, not Unreal dependencies.

## Tests
Invalid floating values, coordinate conversion round-trip tolerance, ID uniqueness, stale-revision distinguishability, deterministic error codes and serialization field validation.

## Status\nPortable base types now exist in `include/world_mechanics/core.hpp` (IDs, vectors, AABB, typed results/errors and floor division). Broader ID generation/serialization policies are still specified only. Compilation and test execution are pending.\n
Specified. Portable source implementation, compiler configuration and passing tests have not yet been added.
