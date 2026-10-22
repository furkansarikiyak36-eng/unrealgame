# WorldScan — Specification

## Responsibility
Acquire bounded observations from a provider and expose a stable engine-neutral query contract. It does not own classification, terrain density or placement policy.

## Query types
- Ray: origin, direction, max range, filters.
- Shape sweep: supported primitive shape and start/end transform.
- Overlap: shape/transform, filters and result cap.
- Surface grid: center, axes, spacing/count and bounded point budget; return position, normal, slope and sample validity.
- Bounds: entity/component AABB and transform when supported.
- Physics observation: supported flags, velocity and awake/simulating status if available.

## Result record
At minimum: hit/miss, target ID, raw tags, contact position/normal/distance, optional bounds/physics facts, query ID, timestamp/tick, source revision if available and budget/truncation status.

## Query behavior
1. Validate finite origin/direction/range; normalize directions when required.
2. Apply filters before semantic use.
3. Enforce per-query/per-frame caps from RuntimeProfiles.
4. Keep raw hit facts separate from semantic classification.
5. Unsupported provider data stays Unknown.
6. Cached observations include source revision/time; invalidate when source revision changes or freshness expires.
7. One camera ray does not prove the continuity, strength or material composition of a whole surface.

## Provider boundary
Portable code defines the query port. Unreal LineTrace/ShapeTrace/Overlap/Bounds/physics calls belong in a future adapter. A deterministic fake provider must make unit tests possible.

## Performance policy
Demand-driven by default. Never scan the whole actor world each frame or cast hundreds of rays per frame without explicit limits/measurement. Local sample grids are bounded. Whole-world Landscape Builder is owned by WorldClassification.

## Tests
Hit/miss, filters, range edge, zero direction, invalid values, result cap, unsupported query type, deterministic tie order, stale result and bounded surface sample cost.

## Status\nA portable provider contract and initial ray/overlap validation wrappers exist. Shape-sweep and surface-sampling service wrappers, provider implementation and engine adapter remain incomplete. Tests have not yet been executed.\n
Specified; engine query adapter and executed tests remain unverified.
