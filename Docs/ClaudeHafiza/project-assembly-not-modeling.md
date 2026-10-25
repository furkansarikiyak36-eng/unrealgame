---
name: project-assembly-not-modeling
description: Core design rule — in the scene nothing is modelled or crafted as a finished prefab; players assemble pieces by hand (log house); raw materials only bind/seal
metadata:
  node_type: memory
  type: project
  originSessionId: 6a44d3af-21f4-4ca5-b383-7b25238c0444
  modified: 2026-10-15T08:35:58.552Z
---

Design rule stated by the user (2026-10-15): "sahnede modelleme olmaz, birleştirmek olur — kütükten ev gibi". Players put structures together piece by piece; no prefab buildings, no finished parts from a recipe. Build mode and prefab stick builds were removed for this reason (commit 7661dc5).

**Why:** physical assembly is the game's core (Sons of the Forest style), see [[project-reference-games]].

**How to apply:** new resources/systems must feed hand assembly. Raw materials (fibre, vine, bark, resin, clay, flint, bone in `EForestItem`, gathered at `ARawMaterialNode`) bind, lash or seal pieces — rope, joint lashing, chinking between logs — rather than produce ready-made panels or walls. Reject or reshape external designs (e.g. a "WattlePanel" item) that bring prefabs back. Integrate with the existing pack (`EForestItem` + `UInventoryComponent`), not a parallel inventory.
