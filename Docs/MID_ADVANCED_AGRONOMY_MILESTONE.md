# Mid-Advanced Agronomy — Milestone 1

## Goal

Integrate crop growth with the existing simulation clock, soil state, and RegionSeason subsystem. This milestone raises the crop model beyond simple GDD + fixed water/nutrient multipliers while keeping the current component architecture.

## Implemented model

- **Phenology:** growing degree days (GDD), crop-specific base/optimum/maximum temperature, stage thresholds, photoperiod and seasonal suitability.
- **Regional fit:** per-crop season and region-type suitability maps. Missing map entries default to neutral suitability (1.0) so old assets do not unexpectedly stop growing.
- **Soil resources:** daily water demand depends on growth stage; actual water uptake is limited by available water above wilting point. N/P/K uptake removes nutrients from the shared soil state.
- **Soil chemistry:** pH suitability reduces growth outside the crop's tolerance band.
- **Competition:** weed biomass scales crop growth down.
- **Temperature damage:** frost and heat damage accumulate; lethal thresholds and prolonged severe stress can kill the crop.
- **Disease:** disease severity responds to wet-day risk and baseline soil disease pressure, with a configurable recovery rate.
- **Yield and quality:** biomass, harvest index, accumulated stress and quality produce an estimated harvest yield.
- **Persistent state:** daily uptake, damage, disease, severe-stress days, quality, yield potential and failure state are held in FPlantState.
- **Integration:** CropFieldActor uses the authoritative USimulationSubsystem daily event, queries URegionSeasonSubsystem at its location, and applies seasonal temperature/precipitation modifiers. It does not create a second clock.

## New crop-definition tuning

- Thermal: `OptimalTemperatureC`, `MinimumDaylightHours`, `MaximumDaylightHours`.
- Water: `BaseWaterDemandMmPerDay`, `PeakWaterDemandMmPerDay`, `WaterDemandStageFactor`.
- Yield: `BiomassPerGDD`, `HarvestIndex`, `MinimumQualityAfterStress`.
- Frost/heat: damage and lethal temperature thresholds.
- Failure: `StressDaysBeforeFailure`.
- Disease: `DailyDiseaseSpread`, `DailyDiseaseRecovery`.
- Competition: `WeedBiomassForMaximumPressure`.
- Nutrition: `NutrientUptakeFractionPerCropCycle`.
- Environment: `SeasonSuitability` and `RegionSuitability` maps.

## Important scope and assumptions

1. Existing PlantDefinition assets need tuning; new map entries default to neutral behavior for backward compatibility.
2. This is a game simulation model, not a validated agronomic forecast. Rates require balancing against game-day duration, soil units, crop scale and desired progression.
3. Soil water uses the current shared FSoilState. Water uptake is subtracted from Soil.Water; N/P/K uptake is subtracted from the corresponding soil pools. Do not apply a second transpiration or nutrient-uptake subtraction elsewhere.
4. CropFieldActor estimates humidity from rainfall because the current SimulationState does not expose relative humidity. A proper weather/humidity model can replace this proxy later.
5. Season temperature offset and precipitation multiplier are applied only when the RegionSeason subsystem returns them. This does not yet rewrite the global weather system for every world actor.
6. The current harvest API still returns biomass for compatibility. `EstimateHarvestYield()` provides biomass × harvest index × quality; a future harvest/processing system should explicitly decide whether inventory receives biomass, usable yield, or both.
7. Seed germination remains a separate component; this milestone does not yet automatically convert seed count into plant population or manage multi-plant density per field.
8. This branch has not been compiled or run in Unreal during authoring.

## Required verification by Claude Code

- Compile with UE 5.8/UHT; fix API and reflection issues without deleting model rules.
- Test no-growth below base temperature and increasing GDD within valid temperatures.
- Test water limitation, nutrient limitation, pH mismatch, weed competition, seasonal mismatch and regional mismatch independently.
- Test frost/heat damage, disease increase/recovery, cumulative-stress failure and mature harvestability.
- Assert water and N/P/K pools never become negative and uptake never exceeds available resources.
- Test missing region/profile and missing season/region map entries.
- Test one daily clock event causes exactly one crop update; verify EndPlay removes the delegate.
- Test old PlantDefinition assets with empty suitability maps remain valid.
- Tune at least one short-cycle crop and one long-cycle crop in a test fixture before claiming balanced gameplay.
