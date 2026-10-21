---
name: feedback-no-keys-to-editor
description: Never send keyboard shortcuts (PressKey) into the open Unreal editor; they can land in the viewport and delete the scene
metadata:
  node_type: memory
  type: feedback
  originSessionId: 6a44d3af-21f4-4ca5-b383-7b25238c0444
  modified: 2026-10-15T13:16:15.748Z
---

Never use SlateInspector `PressKey` (Ctrl+A, BackSpace, Delete, letters) in the user's open editor. Clear text fields with `FillForm` and type with `Type` (which targets its widget).

**Why:** on 2026-10-15 Claude cleared the Output Log box with Ctrl+A + BackSpace; focus was not in the box, so the viewport got ACTOR SELECT ALL + ACTOR DELETE — LogWallTest lost its lights, sky, fog, clouds and PlayerStart, and Claude's script then saved that state. It had to be rebuilt from a backup.

**How to apply:** before any editor automation that saves a map, keep a backup of the map; check the log for unexpected `Cmd:` lines after each run. See [[reference-editor-mcp]].
