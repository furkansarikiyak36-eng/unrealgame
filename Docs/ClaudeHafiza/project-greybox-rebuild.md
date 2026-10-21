---
name: project-greybox-rebuild
description: MyProject2 is a greybox rebuild of the survival/log-building game whose original (art-heavy) code lives in Downloads/MyProject
metadata:
  node_type: memory
  type: project
  originSessionId: 9f227c4e-3bbf-4c74-a5be-b25df61a6efb
  modified: 2026-10-13T05:13:36.412Z
---

Two UE 5.8 projects: `C:\Users\merts\Downloads\MyProject` is the original (Megascans/nature packs, maps NatureTest, MechanicsTest…). `MyProject2` (created 2026-10-12) is a greybox port of the same C++ (Forest* classes, CarvableLog, structural systems), with art replaced by generated assets.

Greybox assets and the `Greybox_Mechanics` map are produced by `Scripts/build_greybox.py` (run via UnrealEditor-Cmd `-run=pythonscript`, editor closed); all mesh lookups go through `ForestTypes.cpp`. Mechanics marked 🔵 (written by a claude.ai session — chinking, structural integrity, stone masonry, bucks) were ported unverified.

Design direction (2026-10-13): "free use, fixed cut points" — what a log becomes (firewood, fire, foundation, wall) is the player's choice via physical cuts, no recipes; cuts only at fixed snap points shown with lines (Sons of the Forest visual language: red dashed = cut, white = connect). Phased plan in `Docs/KUTUK_SISTEMI_PLANI.md`. Every gameplay change gets a headless check in `AMechanicsDemo` auto mode (`-MechanicsDemoQuit` on Demo_WoodChain); the user plays the guided mode.

Status 2026-10-13 evening: planned phases 1-5 implemented and tested headless (74 checks in AMechanicsDemo auto mode + 11 Wood unit tests); only visual results unverified. Open decisions: remove recipe keys 1-7 / inventory, tie trunk length to tree size, whether to merge Wood component system with ACarvableLog. Editor is kept closed during long build/test work and reopened on Demo_WoodChain when finished (user agreed each time).

**Why:** user wants to iterate on mechanics without art dependencies.
**How to apply:** change geometry in the script and rerun it rather than editing assets by hand; keep C++ in sync between projects only if the user asks.
