# Module Dependency Map

CoreContracts is foundational.
- WorldScan → CoreContracts
- TerrainVoxel → CoreContracts
- Excavation → CoreContracts + WorldScan + TerrainVoxel
- Shoring → CoreContracts + WorldScan + TerrainVoxel + Excavation
- StructuralIntegrity → CoreContracts + TerrainVoxel + Shoring
- Survival → CoreContracts + TerrainVoxel + Excavation
- Persistence → snapshot contracts from all state-owning modules
- Validation observes all modules but owns no gameplay rules.

If two modules require each other, move shared data contracts to CoreContracts or introduce an event/port interface. Do not add circular dependencies.
