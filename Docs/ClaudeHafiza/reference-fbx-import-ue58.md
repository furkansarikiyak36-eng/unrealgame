---
name: reference-fbx-import-ue58
description: "UE 5.8 FBX import pitfalls for the user's Blender exports — Interchange drops UCX collision; legacy importer needs scale 100 and a clean folder (Scripts/import_unrealexport.py)"
metadata:
  node_type: memory
  type: reference
  originSessionId: 617998ce-4f9f-4a23-8384-00f3f0e68dc6
  modified: 2026-10-18T22:00:20.094Z
---

Importing the user's Blender FBX files (do/UnrealExport) into UE 5.8, found 2026-10-19:
- Interchange is the default FBX path and ignores `FbxImportUI`: `UCX_` meshes are removed from render but NOT turned into collision (0 convex hulls). Fix: set `Interchange.FeatureFlags.Import.FBX 0` around `import_asset_tasks`, then restore it.
- The legacy importer brings these files in 100× too small (Blender wrote metre values under a cm unit header) and at that size welds vertices (triangles drop). `convert_scene_unit` does not help; `import_uniform_scale = 100` does.
- Re-importing over existing assets keeps their old import settings: delete the destination folder first.
- Verify against Blender measurements (Scripts/analyze_fbx_blender.py) — sizes and triangle counts must match.

Working script: `Scripts/import_unrealexport.py`. Run Python in the editor via the Output Log Cmd box ([[reference-editor-mcp]]); the Main tab must be active (the Fab tab hides the box).
