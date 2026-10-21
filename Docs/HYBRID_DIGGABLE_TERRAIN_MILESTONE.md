# Hybrid Diggable Terrain — Implementation Milestone

## Implemented
- Hybrid brush modes: one grid cell, circular brush, and square brush. Brush weights are applied to authoritative cells; all edits can cross chunk boundaries.
- All cells with suitable soil can be edited. Rock texture is protected; digging also respects a minimum remaining soil layer and the configured bedrock depth.
- Digging updates terrain height/deformation, remaining soil thickness, cumulative excavation depth, compaction, and ground treatment.
- Excavated volume is returned to the caller in cubic metres; inventory is not mutated implicitly.
- Filling accepts loose-soil volume from the caller and reports both consumed loose soil and filled compacted volume. Default loose/compacted expansion ratio is 1.25.
- Tillage reduces compaction and marks the soil as tilled.
- Planting validation checks rock, remaining root depth, compaction, and pH.
- New soil profile and ground-treatment fields are serialized with chunk cells.
- Root-zone field capacity now responds to remaining soil depth in the advanced hydrology model.
- `OnTerrainChanged` reports affected chunks, changed-cell count, and cut/fill volume for rendering/collision adapters.

## Coordinate and volume conventions
- X/Y world coordinates use Unreal centimetres. Terrain heights and excavation depths use metres, matching the hydrology solver's terrain-head convention.
- Cell area = cell size in metres squared.
- Cut/fill volume = depth in metres × cell area.
- Brush radius and per-action depth are clamped by subsystem settings.

## Rendering integration boundary
The current repository has no authoritative terrain mesh/landscape deformation adapter wired to the chunk cell state. The simulation data changes immediately, and `OnTerrainChanged` is the integration point, but visible ground deformation requires a renderer to subscribe and rebuild the affected chunk geometry and collision. This implementation does not claim that the visible landscape is already deforming.

## Known limits
- The brush selects and weights cells; it does not create arbitrary sub-cell triangles or a continuous voxel signed-distance field.
- The current hydrology solver's lateral surface-water exchange remains intra-chunk. Cross-chunk runoff transfer is a separate hydrology improvement.
- Trap triggering/physics is not implemented here. `GroundTreatment::TrapPrepared` is a persistent extension point for a dedicated trap system.
- Unreal Build Tool, UHT, automation tests, and in-game runtime were not available/run in this implementation pass.
