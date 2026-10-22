# WorldScan

**Responsibility:** Convert bounded world queries into explicit observations consumed by gameplay mechanics.

Owns scan request parameters, hit/miss observations, target identity, surface/material tags, normals, distance, classification rules and query budgeting.

Outputs a stable ScanResult/WorldObservation contract for Excavation and placement. A raycast is one possible provider, not the entire design.

**Rules:** Separate raw observation from interpretation; never claim a scan understands all visible world state; define stale-result and query-budget behavior. Unreal line traces belong in an adapter.

**Tests:** Hit/miss, filters, range boundary, invalid normals, classification precedence, query budget and stale observations.

**Status:** Design scaffold only.
