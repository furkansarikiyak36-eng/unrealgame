# WorldEnvironment — Specification

## Responsibility
Publish explicit environment samples (time/weather, water/moisture, light/temperature and localized hazards) to dependent mechanics. It does not own editable density or calculate player survival stats.

## Sample channels
World time/day-cycle and source revision; ambient/underground temperature; humidity/soil moisture; surface/subsurface water level or contact/ingress potential; light and shelter/fire heat contributions; region/chunk ID; sample timestamp and validity/freshness.

Each provider may support only some channels. Missing fields and provenance are explicit.

## Water/terrain separation
Water and soil simulation may live in existing WorldCore/WorldPhaseW4 state. This is not equivalent to voxel density and must not be copied into a second density grid. Interaction with a newly excavated volume needs an exchange/ingress contract, dirty-region policy and persistence decision.

## Ingress/drainage extension
A volume may receive a water-intrusion event when excavation intersects a configured water-bearing region. Drainage channels/pumps are independent mechanics. Flow across chunk seams, time step, conservation approximation, budget and save/load must be specified before calling it implemented.

## Fire/heat/weather
Existing fire/material/weather modules supply heat and ambient inputs through adapters. WorldEnvironment does not duplicate fire state or material interaction matrices.

## Tests
Missing/stale channels, bounded finite inputs, source revision, region boundary, stable repeated samples, chunk unload/reload and authoritative environment round-trip.

## Status
A portable timestamped EnvironmentSample DTO validates optional channels, finite values and range constraints. Weather/water/soil/light providers, time progression, and persistence of global environmental state are not implemented in this module tree.
