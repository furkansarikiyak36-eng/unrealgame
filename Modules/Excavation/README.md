# Excavation

**Responsibility:** Convert player intent and classified scan results into validated terrain edits.

Owns scan-to-dig interaction, target eligibility, excavation request validation, edit preview, depth/shape policy and result events. Uses WorldScan observations and TerrainVoxel commands.

**Rules:** A scan hit is not automatic permission to edit. Preview and commit share validation. Failed excavation cannot leave partial terrain changes. Input and animation stay outside domain policy.

**Tests:** Invalid/out-of-range targets, unsupported material, preview/commit parity, rollback and repeat interaction.

**Status:** Design scaffold only.
