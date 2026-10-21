---
name: reference-ue-python-rotator
description: UE Python unreal.Rotator positional order is (roll, pitch, yaw) — always use keyword args
metadata:
  type: reference
---

In Unreal's Python API, `unreal.Rotator(a, b, c)` takes positional args as (roll, pitch, yaw), not (pitch, yaw, roll) like C++ FRotator. Writing `unreal.Rotator(0, random_yaw, 0)` gives a random pitch: on 2026-10-19 this tipped every scattered tree/shrub/grass in the PerfTest scripts over. Always write `unreal.Rotator(roll=..., pitch=..., yaw=...)`. Same for PCG Transform Points rotation_min/max.

Related: [[reference-editor-mcp]]
