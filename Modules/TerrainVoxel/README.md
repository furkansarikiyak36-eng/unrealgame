# TerrainVoxel

**Responsibility:** Represent editable terrain and layered subsurface state independently from interaction input.

Owns terrain density/layer contracts, chunk coordinates, read/write queries, edit footprints, boundary handling, dirty-region reporting, base-density source boundary and persistent deltas.

**Rules:** Authoritative terrain data is separate from generated render/collision meshes. Coordinate conversions are explicit. Every edit reports affected regions. Storage need not be a dense voxel grid across the entire world.

**Tests:** Chunk boundaries, edit footprints, base-density fallback, persistence round-trip and deterministic samples.

**Status:** Design scaffold only.
