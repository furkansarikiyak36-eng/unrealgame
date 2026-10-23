# Phase 4 — Bounded Environment Simulation

## Added files

- `Modules/WorldEnvironment/include/world_mechanics/world_environment_simulation.hpp`
- `Modules/WorldEnvironment/src/environment_simulation.cpp`
- Regression coverage in `Modules/tests/module_tests.cpp`
- Build registration in `Modules/CMakeLists.txt`

## State ownership

`WorldEnvironmentSimulation` is the authoritative owner of water, soil-moisture, N/P/K, temperature, fuel and burning state for its explicitly registered cells. `TerrainVoxel` remains the owner of editable density; environment state is not copied into a second terrain-density store. `SurvivalExposure` receives an immutable `EnvironmentSample` snapshot from the environment model.

## Algorithms and guarantees

1. **Hydrology:** surface-water flow uses elevation plus water-column head. Per-source aggregate outflow is bounded; inflow at each destination is scaled to its available surface-water capacity. Transfers are applied as deltas, not sequentially mutating a source/destination pair.
2. **Explicit water ledger:** accepted inflow increases external input; overflow is recorded as runoff. Infiltration moves water between surface and soil stores. Evaporation and deep drainage are separately recorded losses. The reported residual is:
   `initial + external input - current stores - evaporation - runoff - deep drainage`.
3. **Soil:** infiltration is limited by surface water, configured rate and remaining soil capacity; drainage and evaporation are bounded by the available store. Nitrogen, phosphorus and potassium leaching are bounded fractions and never create nutrient mass.
4. **Time stepping:** simulation uses a fixed step. Calls have a maximum substep count; remaining elapsed time stays queued. Input that would exceed the queue limit is rejected before adding time.
5. **Fire/heat:** a cell must have sufficient dry fuel to ignite. Burning consumes bounded fuel and contributes heat. Heat transfers across configured edges in deterministic order; wind modifies transfer only in the direction of flow. Newly heated fuel ignites at a threshold crossing, and depleted fires turn off.
6. **Failure handling:** inputs, state bounds, IDs, duplicate edges and work limits return explicit failures. Each fixed step operates on a staged active-region copy and publishes only after its states and totals validate.

## Portable acceptance tests

The test executable now defines eleven test groups, including two new groups for Phase 4:

- `environment hydrology/budgets`: internal transfer conservation, multi-call fixed-step backlog, backlog cap, water overflow/runoff accounting, invalid endpoints/time, infiltration, evaporation, drainage and nutrient leaching.
- `environment fire/survival`: fuel moisture gating, ignition, heat propagation to a linked dry cell, fuel consumption, environment sample validation and survival exposure integration.

**Verification level:** the source and test cases are committed to this feature branch, but the Phase 4 GitHub Actions run must be observed before stating that its build and tests passed. Unreal Engine 5.8 UBT/UHT, Automation, Editor/PIE and Unreal Insights are not claimed as run.

## Not covered yet

- Unreal Landscape, physical-material, weather/time and localized region providers.
- Chunk/region adjacency discovery, cross-chunk water flow and world streaming.
- Pump/drain actor semantics and excavation-to-water ingress events.
- Persistent world-environment schema, migrations and save/load transaction participant.
- Character-stat adapter and gameplay tuning of water/heat/fuel coefficients.
- Production-scale incremental updates. The current active-region state is copied at each fixed step, bounded by `maxCells` and `maxSubstepsPerCall`.

## Phase 4 acceptance gate

- [ ] CMake configure/build passes.
- [ ] All eleven CTest test groups pass.
- [ ] Water ledger residual remains within test tolerance for transfer, runoff, infiltration, evaporation and drainage scenarios.
- [ ] Accumulated time is retained, not silently dropped; excess backlog is rejected without mutation.
- [ ] Moisture blocks ignition; connected dry fuel can ignite deterministically from transferred heat.
- [ ] Engine-facing integration remains a separate gate until UE 5.8 build and Editor/PIE evidence exists.
