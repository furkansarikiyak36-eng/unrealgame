# Persistence — Specification

## Responsibility
Coordinate versioned world snapshots and atomic restore across state-owning modules. It is an orchestrator/codec boundary, not a duplicate gameplay state owner.

## Snapshot payload
- Schema version, build identifier, map/world identity, active RuntimeProfile and grid identity.
- Voxel base-source identity plus sorted sparse density/material overrides and revision.
- Operation/version metadata required to prevent replay of duplicate operations.
- Shoring plans/pieces: stable IDs, definition/type/class identifier, transform, dimensions, material/health/durability, lifecycle state, plan IDs and explicit anchors/sockets.
- Structural graph links, damage/repair data and collapse latches.
- Player transform/respawn/inventory/health/stamina/hunger/thirst through canonical adapters only.
- Survival exposure snapshot and timestamps.
- Stone records only for explicitly supported states (intended: loose/installed/falling). Carried/dragging/fractured/destroyed/terminal states are rejected until schema support is added.
- Legacy log payload only through its existing supported contract.

## Save transaction
1. Every participant emits an immutable snapshot.
2. Validate counts/sizes, finite values, duplicate IDs, map/profile/grid identity and cross-reference integrity.
3. Fail on unsupported runtime states; do not silently truncate arrays or omit actors.
4. Encode and write a new payload/temporary slot.
5. Report success only after storage confirms write; preserve prior save if atomic replacement is supported.

## Load transaction
1. Read and validate checksum/version/map/profile.
2. Migrate supported old schema to current in-memory form without inventing absent data.
3. Preflight required snapshots/references before mutation.
4. Stage voxel and required actor state while retaining old live state.
5. Commit authoritative terrain state; wait until mesh/collision publication is current.
6. Stage/restore physical actors and anchors.
7. Rebuild/validate StructuralIntegrity graph; shoring stays NeedsRevalidation until surfaces are current.
8. Restore Survival/player/environment via adapters.
9. Swap staged state and remove old actors only after all participants commit.
10. On any failure compensate or preserve prior state; return a diagnostic.

## Compatibility
- Schema v2 has no Phase 9 stone-record array. Loading such a payload must not wipe existing live stones because the array is absent.
- Schema v3 introduces stone records; it is not universal actor persistence.
- Map mismatch is rejected unless a versioned migration explicitly permits it.
- Unsupported carry/drag states cause an explicit error.
- Unknown/removed definition refs fail or use a declared migration; never instantiate arbitrary classes based solely on untrusted save data.

## Invariants
Supported state survives save/load; failed load leaves live world unchanged; graph references resolve; terrain mesh/collision is regenerated from density; support is inactive until revalidation; collapsed pieces do not silently revive.

## Caveats
Existing Phase 8/9 Unreal save code is a reference candidate. UE 5.8 build/Automation/PIE were not run here. Active CarvableLog/tree/all carry/drag/rope/vehicle actor families are not all in this payload; they need explicit actor adapters.

## Tests
Round-trip, wrong map, unknown schema, v2 migration, duplicate IDs, bad refs, oversized snapshot, staged-spawn failure, rollback, restored-support revalidation and stone-state validation.


## Status
Portable density snapshot codec (versioned little-endian records, deterministic ordering, bounded decode and checksum) plus the generic staged-restore coordinator exist. This codec covers the voxel density snapshot only; whole-world schema/participants, durable store, legacy migration and actor/player adapters remain unimplemented. New codec tests await CI.
