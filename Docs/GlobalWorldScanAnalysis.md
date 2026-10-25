# Global World Scan & Analysis — Six sensing methods

Branch: `feature/global-world-scan-six-methods-v01`

This layer provides six complementary ways to acquire world information. The methods share one namespace and result contracts; they are not all executed every frame. Callers can use the individual query functions or `WorldScanSystem::ExecuteScanPlan` to dispatch a selective set of queries and collect one `FWorldScanBundle`.

## Six methods

1. **Raycast** — `WorldScanSystem::Trace`: single line trace, raw hit, distance and trace endpoints.
2. **Shape Trace** — `WorldScanSystem::ShapeTrace`: swept sphere, box or capsule; single-hit by default, optional multi-hit.
3. **Overlap Query** — `OverlapSphere` and `OverlapBox`: collect overlapping components/actors in a region.
4. **Terrain / Geometry Sampling** — `SampleSurface`: configurable XY grid of downward traces, returning successful sample points/normals, height range, average normal and maximum sampled slope.
5. **Bounds / Mesh Geometry** — `AnalyzeBounds`: inexpensive actor/component bounds, dimensions, approximate sphere radius and orientation. This deliberately does **not** walk every mesh vertex/triangle.
6. **Physics State Query** — `QueryPhysicsState`: current velocity, angular velocity, rigid-body simulation/awake state and a best-effort current state classification.

## Selective scan plan\n\n`FWorldScanRequest` enables/disables each method explicitly. `ExecuteScanPlan` runs only enabled methods, records which were attempted, and gathers results into `FWorldScanBundle`. A successful raycast can supply the default target actor/component for bounds and physics queries; callers may override that target. The overlap and surface queries use the explicit `QueryCenter`. The function's return value means the plan was dispatched against a valid world; it does not guarantee a collision hit.\n\n## Shared analysis pipeline

`Trace` and `AnalyzeHit` retain the existing single-hit path and object classification. The other methods expose raw or summarized results that downstream systems can combine into a task-specific analysis. Example: a build placement can use overlap + shape trace + surface sampling; a carried-log preview can use raycast + bounds + physics.

## Performance contract

- These APIs are demand-driven. They must not all run for every actor on every tick by default.
- Surface sample grids are clamped to 32 × 32 (maximum 1,024 traces per call). Callers should normally use a small grid such as 3 × 3 or 5 × 5, and request denser sampling only for a real need.
- Shape trace returns one hit by default. Multi-hit must be requested explicitly because result count increases processing work.
- Overlap queries return every matching overlap result; callers should filter and deduplicate actors/components as appropriate.
- Bounds analysis reads engine-provided bounds. It is not a precise mesh-contact or triangle analysis.
- Physics state is a **snapshot**, not historical motion tracking. Determining animated falls that expose neither root velocity nor physics still requires a state tag or typed actor accessor.
- Query channel, collision settings, ignored actors and trace complexity affect the result. A query can only detect objects whose collision setup responds to that query.

## Object and capability tags

Object tags:
- `World.Object.Tree`
- `World.Object.Log`
- `World.Object.Rock`
- `World.Object.Stone`
- `World.Object.Ground`
- `World.Object.Structure`
- `World.Object.RawResource`

State tags:
- `World.State.Falling`
- `World.State.Moving`
- `World.State.Sleeping`

Capability tags:
- `World.Capability.NoSupport`
- `World.Capability.NoSnap`
- `World.Capability.Dig`
- `World.Capability.Harvest`
- `World.Capability.Carry`
- `World.Capability.Interact`

## Integration boundary and validation

This commit adds the shared query APIs; it does not yet replace all placement, digging, support, snap or actor-specific gameplay checks with these APIs. Existing systems should migrate incrementally so their behavior can be compared and regression-tested.

**Validation status:** source files and API wiring were inspected through repository reads. This environment did not run Unreal Build Tool, Editor or PIE. Compilation and in-engine behavior are not yet verified.


## Linked snap consumer

The reusable snap-point resolver is implemented as the independent runtime plugin `Plugins/WorldSnap`. It consumes generic hit/candidate data and does not depend on this scanner module. The project-side `WorldSnapScanBridge` adapts a successful `FWorldScanBundle` raycast plus current `LogSnapSystem` points into the plugin API. It resolves a snap candidate only; placement transforms and structural-support approval remain separate decisions. See `Docs/WorldSnapLinkedPackage.md`.
