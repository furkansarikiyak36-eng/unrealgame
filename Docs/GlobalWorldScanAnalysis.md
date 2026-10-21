# Global World Scan & Analysis — Phase 1–3

Branch: `feature/global-world-scan-analysis-v01`

This change establishes the shared sensing layer before global snap/assembly is refactored.

## Pipeline

1. **World Scanner** — one collision-channel trace; returns raw `FHitResult`, start/end, and trace length.
2. **World Hit Analyzer** — normalizes point, normal, distance, actor/component/physical material, surface profile, object type, physical state, and inferred capabilities.
3. **World Classification** — classifies known gameplay types and supports explicit actor/component tags so new assets can opt in without modifying the scanner.

## Public entry points

- `WorldScanSystem::Trace(...)`
- `WorldScanSystem::AnalyzeHit(...)`
- `WorldScanSystem::ClassifyObject(...)`
- `WorldScanSystem::ClassifySurface(...)`
- `WorldScanSystem::ClassifyPhysicalState(...)`
- `WorldScanSystem::InferCapabilities(...)`

## Classification contract

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

Actor/component tags override generic classification. Existing gameplay actor types are recognized directly. One trace hit is not enough to infer full surface continuity, balance, or a complete slope shape; those require an explicit multi-sample query by the downstream operation. Animated falls that do not move actor root or simulate physics must expose `World.State.Falling` until their actor class provides a proper state accessor.

## Deliberate scope boundary

This phase does not change placement preview, commit, snap resolution, digging, support graphs, or actor movement. It introduces the global scan/analysis contract first. It has not been compiled with Unreal Build Tool or tested in Editor/PIE in this environment.
