# Advanced Water–Soil Simulation — Milestone 1

## Integration status

The advanced solver is the authoritative water/soil step used by `UWorldStateSubsystem::SimulateChunk` on `chatgpt-readonly-work`. The previous `UWaterSimulation` and `USoilSimulation` calls are retained only as a fallback if the advanced solver is unavailable; they are not run alongside the advanced solver. This prevents duplicate infiltration, evaporation, and water movement.

## Implemented processes

1. **Regional climate integration:** chunk precipitation uses the region's seasonal precipitation multiplier and mean annual rainfall relative to a 600 mm baseline; temperature uses regional baseline/elevation plus the seasonal offset, and the regional snow threshold can classify current precipitation as snow.
2. **Persistent layered soil storage:** topsoil, root zone, deep soil, groundwater and snow water equivalent are stored per world cell.
3. **Legacy migration:** when a cell first enters the advanced model, the old normalized `SoilMoisture` value seeds the three soil layers. The legacy field is then maintained as root-zone saturation for existing systems.
4. **Rain/snow input:** precipitation can be added to surface-water volume or snowpack. The world subsystem applies historical catch-up precipitation as liquid because the old weather history stores amounts but not historical rain/snow classification; current precipitation uses the current snow flag.
5. **Snowmelt:** degree-day melt with a daily cap transfers snow water equivalent into surface water.
6. **Infiltration:** soil texture/permeability, top-layer saturation and frozen-soil temperature jointly limit infiltration.
7. **Texture-specific hydraulics:** sand, loamy sand, loam, silt loam, clay loam, clay, organic peat and rock use different capacity/conductivity multipliers. Compaction suppresses infiltration and percolation; organic matter increases water retention; rooting depth scales root-zone storage; shallow bedrock constrains deep storage and drainage. These are game-scale parameters, not measured soil curves.
8. **Vertical redistribution:** water drains from topsoil to root zone, root zone to deep soil, then deep drainage to a groundwater store. Layer capacities and rates are configurable.
9. **Evaporation and transpiration:** atmospheric demand is temperature- and humidity-dependent; surface water evaporates first, then topsoil, and vegetation cover scales root-zone transpiration.
10. **Surface routing:** neighbour exchange is calculated from terrain elevation plus surface-water depth, with a two-phase delta buffer and a maximum transfer fraction to avoid order-dependent updates and negative volumes.
11. **Nutrient leaching:** water percolation reduces the existing nitrogen, phosphorus and potassium pools at configurable relative rates.
12. **Erosion pressure:** surface flow, local slope and vegetation cover produce a persistent erosion index and suspended-sediment proxy. It does not yet physically lower terrain or transport sediment between cells.
13. **Diagnostics and events:** each simulation step returns total water before/after, infiltration, ET, melt, vertical drainage, surface flow, mean root-zone saturation and water-balance error. World events are broadcast for material hydrology changes.
14. **Persistence:** the layered water state, snowpack, soil temperature, cumulative fluxes, erosion index and balance error are serialized with each chunk cell.
15. **Bounded catch-up:** elapsed simulation intervals longer than one day are processed as one-day substeps rather than clamped and discarded.

## State units

- Surface water is stored in cubic metres per cell.
- Soil layers, snowpack and groundwater are stored as water-equivalent millimetres per cell area.
- `SoilMoisture` remains a dimensionless compatibility field representing root-zone saturation (0–1).
- Existing nutrient values do not declare calibrated physical units. Leaching is therefore a game-scale relative-pool model and must be balanced before interpreting it as kg/ha.

## Important model boundaries

- Soil hydraulic properties use per-cell texture, compaction, organic matter, rooting depth and bedrock depth, combined with solver-wide tuning values. Per-cell calibrated retention curves, van Genuchten parameters, macropore flow and capillary rise are not implemented.
- Groundwater receives deep drainage but has no lateral aquifer flow, spring emergence or water-table feedback yet.
- Surface flow remains within the chunk grid. Cross-chunk edge exchange and watershed-scale routing are not implemented in this milestone.
- Erosion currently tracks pressure/index and a sediment proxy; it does not deform terrain or move sediment to downstream cells.
- Weather humidity is supplied as a bounded constant by the world subsystem until the weather model exposes relative humidity. Historical weather catch-up cannot reconstruct whether each past precipitation event was snow; historical rainfall is currently liquid and the current weather event uses the current snow flag. Historical catch-up also applies the current regional season's climate scale rather than reconstructing each missed day's regional season.
- Snowfall is modeled when the current weather state identifies snow and the temperature condition is met; a historical snow record needs its own persisted weather-event data.
- No claim is made that UE 5.8 compilation or automation tests have run in this authoring environment.

## Required Claude Code validation

- Compile under UE 5.8/UHT and fix integration errors without removing the physical rules.
- Run the `MyProject2.Hydrology.MassBalance`, `MyProject2.Hydrology.SnowpackAndMelt`, `MyProject2.Hydrology.PersistenceRoundTrip` and `MyProject2.Hydrology.SoilTextureResponse` automation tests.
- Add tests for dry-soil infiltration, saturated-soil infiltration slowdown, frozen-soil inhibition, nutrient non-negativity, lateral-flow conservation, 1.0-day versus multi-day catch-up, serialization round-trip and legacy cell migration.
- Check water balance over 1, 7 and 30 simulation days. Water balance should equal initial water plus explicit precipitation inputs minus modeled evapotranspiration and any future explicitly modeled external exports.
- Profile large chunk counts and ensure the event system does not become the dominant cost.
- Tune cell area and terrain-height units against `FWorldChunkSettings`; all surface-flow head terms must use metres consistently.
