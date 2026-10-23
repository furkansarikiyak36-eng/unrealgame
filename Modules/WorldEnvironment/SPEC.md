# WorldEnvironment — Specification

## Responsibility

Own the portable simulation state for an explicitly admitted active region and publish timestamped environment samples to dependent mechanics. It does not own editable terrain density, mesh/collision products, or player health/stamina.

## Authoritative stores and units

- Geometry is expressed in meters; water and soil quantities are stored as cubic meters (m³) per cell.
- Surface water and soil water are distinct bounded stores with explicit capacities.
- Each cell owns soil N/P/K nutrient mass (kg), temperature (°C), fuel mass (kg), fuel moisture and burning state.
- Terrain elevation and cell area define hydraulic head and convert surface water volume to an approximate water-column height.
- The cell graph is explicit: a hydraulic/thermal connection does not imply spatial adjacency unless the provider defines it.
- Voxel density remains authoritative in `TerrainVoxel`; water and fire are state layers, not a second density grid.

## Implemented portable behavior

`WorldEnvironmentSimulation` provides:

- Deterministic hydraulic transfer using terrain elevation plus surface-water height. Source outflow is limited to a configured fraction per fixed step; destination inflow is scaled to fit its capacity.
- Explicit rainfall/inflow acceptance and overflow runoff accounting.
- Bounded infiltration from surface water into soil water, temperature-adjusted evaporation, deep drainage, and moisture-driven nutrient leaching.
- A fixed-step accumulator with a maximum number of substeps per call. Unprocessed time is retained as backlog; new time input is rejected if it would exceed the configured backlog cap.
- Deterministic heat transfer over configured links, directional wind influence, fuel consumption, moisture-gated ignition and fuel-exhaustion extinguishing.
- One shared cell snapshot that can produce a validated `EnvironmentSample` for `SurvivalExposure`.
- Input validation, finite-value checks, cell/link/time budgets and a cumulative water-balance ledger.

## Water accounting invariant

For the active region:

`initial water + accepted external water = current surface/soil water + cumulative evaporation + runoff + deep drainage`

Internal surface-to-soil infiltration and cell-to-cell transfers do not create or destroy water. `WaterBalanceErrorM3()` exposes the numerical residual for tests and diagnostics. Nutrient leaching removes nutrients from the modeled soil stores; it does not implicitly fertilize downstream cells.

## Determinism and cost limits

Cells are stored by stable ID and connections are normalized and sorted. Each fixed step stages the bounded region and commits only after validation. Cell, connection, fixed-step and queued-time budgets are explicit. The current implementation is designed for modest active regions; copying the whole active-region map every substep is deliberately bounded but is not yet a production-scale streaming solution.

## Explicit limitations

This is a game-oriented approximation, not a validated fluid-dynamics or wildfire model. Conductance, evaporation, infiltration, heat transfer and burn rates require calibration against intended gameplay. The portable module currently has no Unreal Landscape/physical-material/weather provider, spatial-neighbor discovery, chunk-boundary exchange, pump/drainage actor, asynchronous region scheduler, or durable environment persistence. Unreal Engine 5.8 UBT/UHT, Automation and Editor/PIE are not yet verified.

## Acceptance tests

The portable test suite covers:
- closed water balance under internal transfer;
- explicit overflow runoff;
- retained fixed-step backlog and rejection beyond the queue cap;
- infiltration, evaporation, drainage and nutrient loss accounting;
- wet-fuel ignition rejection;
- deterministic connected-cell heat spread and ignition;
- shared environment samples feeding the survival exposure model;
- duplicate/missing IDs and invalid time rejection.
