---
name: feedback-check-open-map
description: Editor scripts must check which map is open before saving to a fixed map path — the user switches maps between my steps
metadata:
  node_type: memory
  type: feedback
  originSessionId: 617998ce-4f9f-4a23-8384-00f3f0e68dc6
  modified: 2026-10-20T06:54:49.838Z
---

Before any editor script saves a map to a fixed path (`save_map(world, '/Game/Maps/PerfTest')`), check that the open world is that map, and stop otherwise. Also re-check the open map before multi-step work, since the user opens other maps in the same editor while I work.

**Why:** 2026-10-20 the user had switched to KitShowcase; my sound-import script saved the open world to /Game/Maps/PerfTest and overwrote the meadow test map with a KitShowcase copy. It was recovered only because an editor autosave (Saved/Autosaves/Game/Maps/PerfTest_Auto2.umap) existed.

**How to apply:** new scripts that save a map start with the `_OPEN = ...get_editor_world()` guard used in Scripts/*.py (21 scripts have it). For recovery, autosaves live in Saved/Autosaves/Game/Maps/<Map>_AutoN.umap and can be copied over the map with the editor closed. See [[reference-editor-mcp]].
