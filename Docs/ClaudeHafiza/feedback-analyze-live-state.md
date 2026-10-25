---
name: feedback-analyze-live-state
description: "When asked to analyze the project/scene, measure the current live state (editor via MCP, disk), not findings from PROJE_GUNLUGU or memory"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 617998ce-4f9f-4a23-8384-00f3f0e68dc6
  modified: 2026-10-17T04:09:51.207Z
---

"Analiz et" means analyze what is there now: read the open editor's actors via MCP ([[reference-editor-mcp]]), the files on disk, captures. Do not restate earlier log/memory findings as current.

**Why:** 2026-10-17 the user rejected an analysis built partly from Docs/PROJE_GUNLUGU.md and memory: "önceki hafızaya göre analiz yapıyorsun yapma, şu anı analiz et".

**How to apply:** Dump the scene live (ProgrammaticToolset.execute_tool_script batching SceneTools.find_actors + ActorTools get_actor_transform/get_actor_bounds + ObjectTools.get_properties), take CaptureViewport images, and report only measured facts. Old log entries may be used just to say what changed.
