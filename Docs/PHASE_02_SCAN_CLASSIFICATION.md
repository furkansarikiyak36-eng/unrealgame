# Faz 2 — LOW_MID World Scan + Classification

**Branch:** `feature/phase-02-scan-classification-lowmid`  
**Base:** Faz 1 runtime contracts  
**Target:** Unreal Engine 5.8 / C++  
**Status:** Implementation draft; UBT, Automation, Editor/PIE and landscape gameplay validation have not been run in this authoring environment.

## Goal

Faz 2 supplies the first concrete implementation behind the Faz 1 interfaces:

- a demand-driven query subsystem that implements `IWorldScanProvider`;
- a bounded region-classification map implementing `IWorldClassificationSource`;
- deterministic priority and boundary behavior;
- automated test definitions and a build/test runner.

This is the sensing/classification slice. It does not yet generate smooth voxel terrain, alter Landscape height/weight data, or route all player actions through the new scanner.

## What was added

| File | Responsibility | Validation state |
|---|---|---|
| `WorldCore/WorldClassificationSubsystem.h/.cpp` | Bounded region grid, actor/component region tags, landscape-layer marker tags, manual AABB overrides, limited center-ray fallback, point query/revision | Written; UE build/test pending |
| `WorldCore/WorldScanSubsystem.h/.cpp` | Adapter that implements `IWorldScanProvider` on top of the shared scan core and classification map | Written; UE build/test pending |
| `WorldScanSystem.h/.cpp`, `WorldScanPlan.cpp`, `WorldScanQueries.cpp` | Reused scan core for hit analysis, actor classification, line/shape trace, overlap, surface sample, bounds/physics snapshot and selective scan plan | Imported from scanner drafts and adapted; UE build/test pending |
| `ForestCharacter.cpp` | `TraceFromView` uses the shared scanner when available; wall-plan ghost component ignore behavior is preserved | Source path updated; UE build/PIE pending |
| `WorldCore/WorldSystemContracts.h` | Adds a hard limit for classification geometry probes and makes classification-map builds explicitly mutating | Updated |
| `Tests/WorldScanClassificationTests.cpp` | Region marker policy, manual grid overrides/boundary rules, demand-driven scanner smoke tests | Three tests written; not run |
| `Scripts/run_phase2_tests.sh` | UE 5.8 build, runs all `MyProject2.World` tests and verifies Phase 1 + Phase 2 test names/results | Bash syntax / Unreal execution pending |

## Classification priority

Highest priority wins; candidates use stable lexical keys rather than world actor iterator order. If equal-priority markers disagree on the kind, the cell fails closed to `Transition` rather than picking an editable label.

1. **Manual override** — an explicit world-space AABB registered using `SetManualOverride`.
2. **Landscape-layer marker** — explicit `World.LandscapeLayer.*` actor/component tag.
3. **Region marker** — explicit `World.Region.*` actor/component tag.
4. **Geometry probe** — a bounded vertical ray in an otherwise unknown cell. Untagged Landscape/static collision is classified as `StaticGeometry`, never editable by inference.
5. **Unknown** — retained when evidence is absent or the bounded probe budget has been exhausted.

Supported region markers:

- `World.Region.EditableVoxel`
- `World.Region.StaticGeometry`
- `World.Region.Transition`
- `World.Region.Water`
- `World.LandscapeLayer.EditableVoxel`
- `World.LandscapeLayer.StaticGeometry`
- `World.LandscapeLayer.Transition`
- `World.LandscapeLayer.Water`

### Conservative coarse-cell rule

The classifier does not silently extend a small region marker to an entire larger voxel cell. If an explicit marker only partially covers a cell, that cell becomes `Transition`. This prevents an editable marker on part of a cell from authorizing digging through neighboring static geometry.

A marker that only partially covers a region creates `Transition`; equal-priority conflicts between differing markers also create `Transition`. The grid uses the requested world-space bounds and per-axis region size. A point query inside the generated bounds returns the cached cell; a point outside the cache returns `false`. An in-grid `Unknown` classification is still a successful lookup, and consumers must inspect `Kind`.

**Landscape limitation:** `World.LandscapeLayer.*` is a stable actor/component-tag convention in this phase. This implementation does not read painted Landscape weightmap layers. Real per-weightmap sampling and an editor visualization of the classification grid remain later tasks.

### Cost bounds

- Each build validates the requested grid count against `MaxRegions`.
- Center-ray geometry fallback is bounded by `MaxGeometrySamples` (default 1,024; hard maximum 50,000).
- Geometry probing is not a per-frame Tick loop; it runs only when `BuildClassification` is called.
- Query methods run only when requested.
- Surface sample queries are capped by both request limits and `UWorldRuntimeProfileSubsystem::MaxSurfaceSamplesPerAxis`; initial LOW_MID allows 5 × 5 and HIGH_END 9 × 9.
- Shape sweep is single-hit unless `bMultiHit` is explicitly requested.

## Scanner behavior

- `AForestCharacter::TraceFromView` now calls this scanner for its ordinary first-person view ray. The pre-existing `bIgnorePlans` path still ignores each wall-plan ghost component; a direct trace remains as a defensive fallback only if the subsystem is unavailable.
- `WorldScanSubsystem` reuses the existing `WorldScanSystem` hit analysis and query implementations rather than implementing a second independent set of ray/sweep/overlap/physics routines. It adapts those results to the Faz 1 contracts.
- `Trace`: returns `true` only on a blocking actor hit. `OutHit.bHit` describes the hit and the result includes the raw hit, object class name, region kind and capabilities. The request controls whether physical material lookup is needed and can ignore specific ghost components.
- `ShapeTrace`: returns `true` when a valid query is executed; `OutResult.bSucceeded` distinguishes an executed query from invalid input. A valid no-hit query has zero hits.
- `QueryOverlaps`: returns `true` when a valid overlap query is executed; inspect `Overlaps` for matches.
- `SampleSurface`: returns `true` when at least one requested downward sample hit a surface. The result reports hit points/normals, min/max Z and maximum slope angle.
- `AnalyzeActor`: returns bounds/dimensions, actor origin, and a physics velocity/awake snapshot where a primitive component is available.

Capabilities begin with the shared `WorldScanSystem` object/physics analysis and are constrained by the region map. `World.Capability.NoSupport`, `NoSnap`, and `NoDig` are opt-outs. **Digging is opt-in**: a hit must both be inside an `EditableVoxel` region and carry `World.Capability.Dig`; a region label by itself does not authorize destructive interaction. Moving/falling physics targets are not considered safe supports.

## Tests

Added automation test names:

- `MyProject2.World.Classification.RegionTagPolicy`
- `MyProject2.World.Classification.ManualOverrideGrid`
- `MyProject2.World.Scan.DemandDrivenQueries`

The phase runner also verifies these Phase 1 names:

- `MyProject2.World.RuntimeProfile.DefaultProfilesValidate`
- `MyProject2.World.RuntimeProfile.ConfigureAndLock`
- `MyProject2.World.Contracts.ValidateRequestsAndBuffers`

Run on Windows Git Bash with Unreal Engine 5.8 installed, editor closed:

```bash
bash Scripts/run_phase2_tests.sh
```

A green source-level review is not a substitute for UBT/UHT or running Unreal Automation. The PR should remain draft until the build and all six tests pass.

## Explicitly not in this phase

- Read/write of painted Landscape weightmap data.
- Landscape Builder viewport overlay/editor brush.
- Runtime smooth voxel meshing and collision generation.
- World terrain mutation/dig tool and mesh update queue.
- Automatic binding into `AForestCharacter` input/placement.
- Voxel trench shoring, support damage/collapse, survival and save/load integration.
- Performance benchmark on Low–Mid or High-End hardware.

## Next acceptance step

Run `Scripts/run_phase2_tests.sh` in the UE 5.8 project environment. Fix every compiler/UHT/test error before advancing to Faz 3. A successful Phase 2 test pass would validate the query/classification foundations; it still would not validate smooth voxel rendering or excavation.
