# Mechanics Modules — Pre-Unreal System

This directory is the mechanics-first working system, not a phase archive and not yet an Unreal-integrated implementation.

## Rules
- Organize by mechanic/responsibility, never by the phase in which a file was first written.
- Keep domain rules and data contracts independent of Unreal Engine APIs where practical.
- Each module owns its rules and state transitions; other modules use documented contracts.
- Unreal-specific actors, components, reflection, assets, collision queries and world lifecycle belong in a future adapter layer.
- A design document is not proof of implementation or test completion.

## Modules
- CoreContracts — shared IDs, units, results and invariants.
- WorldScan — world observations and classification.
- TerrainVoxel — layered terrain/voxel data and edits.
- Excavation — scan-to-dig interaction and validated terrain edits.
- Shoring — support planning and placement constraints.
- StructuralIntegrity — support graph, damage and collapse decisions.
- Survival — underground exposure and survival effects.
- Persistence — validated snapshots and transactional restore.
- Validation — cross-module scenarios and readiness gates.

See Docs/ARCHITECTURE.md, Docs/DEPENDENCY_MAP.md and Docs/INTEGRATION_STATUS.md.
