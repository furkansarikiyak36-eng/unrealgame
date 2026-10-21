# World Snap — linked package

**Target:** Unreal Engine 5.8 / C++  
**Package:** `Plugins/WorldSnap`  
**Integration branch:** `chatgpt/world-snap-linked-package`

## Responsibility boundary

`WorldSnap` is a standalone Unreal runtime plugin. It depends only on Core, CoreUObject and Engine. It does not include `WorldScanSystem.h`, `ACarvableLog`, character code, or any gameplay actor. This keeps the resolver reusable and prevents a dependency cycle.

The project-side adapter, `WorldSnapScanBridge`, is in the main game module. Its data flow is:

1. `WorldScanSystem::ExecuteScanPlan` acquires the hit and normalized analysis.
2. The bridge validates that the raycast was attempted and hit a log, then asks the existing `LogSnapSystem` to generate its stable candidate points.
3. The adapter maps those points into the plugin's generic `WorldSnap::FPoint` contract.
4. `FWorldSnapResolver` filters candidates by group, distance, surface-normal compatibility and optional axis error; then deterministically returns the best point.
5. Placement mode, transform construction, occupancy and structural-support validation remain the responsibility of the gameplay/placement layer. Resolving a snap point is not a placement approval.

## Public API

- `WorldSnapTypes.h`: generic candidate, query and result data.
- `WorldSnapResolver.h`: `FWorldSnapResolver::Resolve`.
- `WorldSnapScanBridge.h`: game-specific scan-to-log adapter.

The result preserves the source candidate index, point ID/group, position, normal, axis, distance, normal alignment, axis error and score. Setting `RequiredGroup` restricts the query to a named group such as `Top`, `Front`, or `EndA`; `NAME_None` disables group filtering.

## Selection behavior

- The normal dot product is checked against a configurable threshold (default 0.25).
- The maximum search radius is configurable (default 125 cm).
- The optional axis constraint limits error along each candidate's axis (default 45 cm in the log bridge).
- Candidate generation order is the stable tie-breaker. Generate points deterministically.
- Invalid query data, empty candidate lists and no compatible point produce distinct result statuses.
- No world trace, overlap, actor iteration or tick work is performed by the resolver itself; it operates on supplied data.
- Spacing is intentionally owned by the candidate provider, not globally hard-coded in the resolver. The current log adapter uses existing log-module snap points; a plank adapter can provide a 20 cm lattice or any other chosen interval without changing this plugin.

## Minimal usage

The scanner's request should enable raycast and populate a `FWorldScanBundle`; then:

```cpp
WorldScanSystem::FWorldScanRequest ScanRequest;
ScanRequest.bRunRaycast = true;
ScanRequest.bRunBoundsAnalysis = false;
ScanRequest.bRunPhysicsQuery = false;
ScanRequest.TraceStart = ViewStart;
ScanRequest.TraceEnd = ViewEnd;

WorldScanSystem::FWorldScanBundle Scan;
WorldScanSystem::ExecuteScanPlan(GetWorld(), ScanRequest, this, nullptr, nullptr, Scan);

WorldSnapScanBridge::FResolvedLogSnap Resolved;
WorldSnapScanBridge::FLogSnapSettings Settings;
if (WorldSnapScanBridge::ResolveLogSnap(Scan, Settings, Resolved))
{
    // Resolved.LogPoint is the candidate; placement/support rules still decide the final transform.
}
```

## Tests and validation

Automation tests were added for nearest compatible candidate selection, stable tie-breaking and required-group filtering. They are source-level test definitions and have not been run here. This change has not been compiled with UE 5.8 UBT or validated in Editor/PIE; plugin discovery, C++ compilation, the automation tests and the bridge against real log assets remain required checks.
