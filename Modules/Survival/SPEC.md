# Survival — Specification

## Responsibility
Translate approved actions and environmental samples into exposure/effort events while preserving existing canonical player-state ownership.

## Canonical-owner rule
Existing `USurvivalComponent` is the candidate owner for health, stamina, hunger and thirst. Do not create a competing stat system. A portable module exposes a port/adaptor to the canonical stats rather than cloning ownership.

## Operation effort events
Configurable costs for tool swings, heavy excavation, shoring place/carry/remove/repair and heavy-resource carrying. Charge/refund timing is explicit: rejected edits cannot consume irreversible resources unless a deliberate gameplay rule says so.

## Exposure sample
Timestamp/freshness, ambient/underground temperature, moisture/wetness, light, water contact/depth, underground flag, player depth, shelter/roof, fire/heat contribution and source IDs where available. Missing values are explicit, never silently mapped to safe values.

## Exposure state
Accumulated thermal stress, wetness, darkness exposure, water exposure and recovery/decay counters. Update in bounded fixed/sliced time steps; a long frame stall must not cause unbounded numerical jumps.

## Rules
- Environment samples come from WorldEnvironment/provider adapters.
- Emit effects/events (cold stress, wetness penalty, darkness feedback, water movement cost); do not reach into UI/rendering.
- Underground access/escape entities are owned by placement/shoring/movement systems.
- Oxygen/gas is an optional future extension, not a core acceptance dependency.
- Snapshot schema stores accumulators and timestamps needed for deterministic restore.

## Tests
Effort cost by operation, rejected-action refund, time-step clamp, freshness, missing-data policy, accumulation/recovery, depth/water transitions and save/restore.

## Status
Specified; environmental provider and character adapter are not verified end-to-end.
