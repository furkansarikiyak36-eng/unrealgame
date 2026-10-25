# World Snap — linked package

**Target:** Unreal Engine 5.8 / C++  
**Package:** `Plugins/WorldSnap`  
**Integration branch:** `feature/sotf-mechanics-expansion`

## Responsibility boundary

`WorldSnap` is a standalone Unreal runtime plugin. It depends only on Core, CoreUObject and Engine. It does not include `WorldScanSystem.h`, `ACarvableLog`, character code, or any gameplay actor. This keeps the resolver reusable and prevents a dependency cycle.

The project-side adapter, `WorldSnapScanBridge`, remains in the game module for consumers of `FWorldScanBundle`. In the active player log placement path, `FLogSockets` also maps the existing `LogSnapSystem` candidates to generic `WorldSnap::FPoint` records and calls `FWorldSnapResolver` directly. This uses the common candidate resolver without moving all gameplay-specific placement rules into the plugin.

The resolver selects a compatible snap candidate; it does not by itself approve structural support or perform the final placement.

1. `WorldScanSystem::ExecuteScanPlan` acquires the hit and normalized analysis.
2. The bridge validates that the raycast was attempted and hit a log, then asks the existing `LogSnapSystem` to generate its stable candidate points.
3. The adapter maps those points into the plugin's generic `WorldSnap::FPoint` contract.
4. `FWorldSnapResolver` filters candidates by group, distance, surface-normal compatibility and optional axis error; then deterministically returns the best point.
5. Placement mode, transform construction, occupancy and structural-support validation remain the responsibility of the gameplay/placement layer. Resolving a snap point is not a placement approval.

## Public API

- `WorldSnapTypes.h`: generic candidate, query and result data.
- `WorldSnapResolver.h`: `FWorldSnapResolver::Resolve`.
- `WorldSnapTwoAnchorSolver.h`: `FWorldSnapTwoAnchorSolver::Solve` for a rigid piece constrained by two endpoint anchors.
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

## Two-anchor endpoint solving

`FWorldSnapTwoAnchorSolver` receives start/end anchor records with IDs, semantic kinds, endpoint-centre positions, availability and load-bearing flags. The query provides piece length and local X bounds. The solver rejects invalid/unavailable anchors, non-load-bearing endpoints, duplicate anchor IDs, endpoint distance that does not match piece length, and pitch outside the configured range. A successful result contains the transform and the two solved endpoint positions.

The active sloped-log path computes a ground-tangent endpoint and an endpoint on an anchored post top, then calls the solver. The gameplay path still performs support preview and commit-time support evaluation. The generic solver does not determine every legal anchor-kind pairing for every building type; those policies belong to gameplay adapters.

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

Automation tests cover nearest compatible candidate selection, stable tie-breaking, required-group filtering and six two-anchor cases (level placement, 30-degree slope, length mismatch, non-load-bearing anchor, duplicate anchor ID and pitch-range rejection). The headless log verifier now expects nine WorldSnap tests in total. The tests remain source definitions until executed; this branch has not been compiled with UE 5.8 UBT or validated in Editor/PIE, so plugin discovery, C++ compilation and real-mesh gameplay validation remain required.
