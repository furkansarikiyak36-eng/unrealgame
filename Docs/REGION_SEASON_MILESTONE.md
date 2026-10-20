# Region and Season — Milestone 1

## Goal

Add a dependable seasonal calendar and explicit spatial region recognition without replacing or duplicating the existing world clock, water/soil simulation, or terrain system.

This is a foundation milestone, not a claim that all environmental effects are already integrated into gameplay.

## What this milestone adds

- Four deterministic calendar seasons: Spring, Summer, Autumn, Winter.
- A configurable calendar driven by the existing USimulationSubsystem day event.
- AWorldRegionVolume: a non-blocking spatial volume with region ID, display name, type, priority, and optional climate profile.
- URegionSeasonProfile: data-driven regional baseline climate plus seasonal modifiers.
- URegionSeasonSubsystem: location-to-region resolution and a climate snapshot API.
- Overlap policy: highest priority wins; equal priority uses a stable path-name tie-breaker.
- Unknown locations are reported explicitly; they are not silently assigned a fabricated region.
- Seasonal transition and day-advanced delegates for future integrations.

## Deliberate scope limits

1. Season changes are calendar state only at this stage. The module exposes regional temperature offsets, precipitation multipliers, wind multipliers, snow-threshold offsets, and daylight duration. It does not yet override the existing global weather simulation.
2. Region profiles must be assigned to placed region volumes. Code cannot infer authored geography from an empty level. The level-design pass must place and configure a volume for every region that should be recognized.
3. No digging implementation is added in this milestone. The next terrain interaction milestone must enforce the initial tool/material limit: loose soil may be dug; solid stone and bedrock cannot be dug with the initial tool. Stone mining is a separate future capability and must not fall through to soil excavation.
4. No farming, survival, raid, or terrain deformation behavior is silently changed here. These systems should consume regional climate through explicit adapters in later milestones.

## Integration contract for Claude Code

1. Keep USimulationSubsystem as the authoritative clock. Do not add a second time source.
2. Integrate this module in the chatgpt-readonly-work branch only. Do not edit or merge into main.
3. Build and fix UHT/C++ integration issues without deleting domain rules or reducing feature scope. Report any design change before applying it.
4. After compilation, add automation tests for day 1 = Spring; season boundaries and year wrap; AdvanceDays and normal clock ticks updating the calendar; point-inside and outside-volume lookup; priority and stable overlap tie-breaks; missing profile and missing modifier fallbacks.
5. Add level test fixtures or a documented setup procedure for at least two distinct regions with different climate profiles.
6. Do not claim regional weather effects work in gameplay until UWorldStateSubsystem/chunk weather and relevant systems consume the snapshot and are tested.

## Initial tuning convention

Use neutral values as safe defaults. A missing seasonal modifier means no temperature offset, precipitation multiplier 1.0, wind multiplier 1.0, and 12 daylight hours. Climate profiles should be tuned per region, not hard-coded into the subsystem.

## Acceptance criteria

- One authoritative calendar.
- Region identity can be queried from any world position.
- Seasonal data is authored as data, not scattered across conditionals.
- Results are deterministic and robust to overlapping volumes.
- Existing water, soil, terrain, survival, and construction behavior remains unchanged until explicitly integrated.
- Compile and test results are recorded separately from in-game visual verification.
