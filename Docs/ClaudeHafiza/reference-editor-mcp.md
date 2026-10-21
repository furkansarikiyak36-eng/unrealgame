---
name: reference-editor-mcp
description: "How Claude can drive the open Unreal Editor itself — its MCP server on 127.0.0.1:8000, client Scripts/ue_mcp.py, running Python via the Output Log box, Live Coding binding limits"
metadata:
  node_type: memory
  type: reference
  originSessionId: 6a44d3af-21f4-4ca5-b383-7b25238c0444
  modified: 2026-10-20T05:30:03.274Z
---

The user expects Claude to check scenes and run things in the open editor itself rather than hand over commands ("sahneyi kontrol edebiliyorsun, derleyebiliyorsun da, yapsana", 2026-10-15).

- UE 5.8 ModelContextProtocol plugin serves MCP on `http://127.0.0.1:8000/mcp` while the editor runs. Client: `python Scripts/ue_mcp.py tools/call '<json>' [script.py]` (meta-tools: list_toolsets, describe_toolset, call_tool).
- Useful toolsets: SceneTools (find_actors needs name/tag/collision_channels), ObjectTools (`set_properties` takes `values` as a JSON *string*), AssetTools.save_assets, EditorAppToolset (CaptureViewport needs all args, StartPIE/StopPIE), ProgrammaticToolset.execute_tool_script (sandbox: only json/math/re…, no `unreal`).
- Full editor Python: SlateInspectorToolset `Type` into the Output Log box (ref from Snapshot, e.g. `tb2`) — in Cmd mode `py "<file>"`, in Python mode the bare path — submit=true. Clear the box first with `FillForm` (value ""), never with `PressKey` Ctrl+A/BackSpace: PressKey goes to whatever has focus, and twice it hit the level viewport = ACTOR SELECT ALL + ACTOR DELETE (wiped LogWallTest's lights/sky, 2026-10-15). Type itself targets the box. Python-only API names differ in UE 5.8 (GeometryScript_MeshEdits, GeometryScript_NewAssetUtils, get_num_triangle_i_ds…): probe with dir() first.
- Safer submit (2026-10-20): `FillForm` the whole command into the box, then `Type` with text "" and submit=true — viewport navigation keys otherwise leak "w/a" into the box. The box ref changes per session (tb1 after a fresh start); re-find it with `Snapshot {"ref":""}`. ProfileGPU opens a GPU Visualizer tab that hides the box: click the level tab first. Minimized editor = 3 FPS throttle: measurements are invalid (FrameTime 333 ms) — restore the window and set EditorPerformanceSettings bThrottleCPUWhenNotForeground=False via ObjectTools.
- Classes/enums added by Live Coding have no Python bindings until the editor restarts; set their properties via ObjectTools instead. Viewport capture fails while the editor window is minimized.
- Another AI agent also uses this MCP server (it bloated Greybox_Mechanics); see [[project-greybox-rebuild]].
