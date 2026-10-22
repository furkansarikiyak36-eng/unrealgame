# Survival

**Responsibility:** Calculate survival consequences caused by underground and excavation conditions.

Owns exposure inputs, accumulation/decay, threshold transitions, effects/events and policies for unavailable environmental samples.

**Rules:** Calculations are separate from UI/animation. Missing data has an explicit policy and is never silently treated as safe. Time-step/update cadence is explicit. This module does not own all character survival or global weather.

**Tests:** Thresholds, decay/recovery, paused time, missing data, depth changes and save/restore.

**Status:** Design scaffold only.
