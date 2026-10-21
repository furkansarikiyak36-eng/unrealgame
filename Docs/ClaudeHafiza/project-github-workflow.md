---
name: project-github-workflow
description: "Two-handed workflow — mechanics code arrives via GitHub (integrate/web-plan); Claude pulls, builds, wires in the editor, tests in PIE, pushes back"
metadata:
  node_type: memory
  type: project
  originSessionId: 617998ce-4f9f-4a23-8384-00f3f0e68dc6
  modified: 2026-10-20T08:56:27.283Z
---

From 2026-10-20 the user develops new mechanics elsewhere (they or another AI tool write the code) and pushes to GitHub, branch `integrate/web-plan` of furkansarikiyak36-eng/unrealgame (public repo). When they say "X mekaniğini ekledim / entegre et", Claude: `git pull` → build (editor closed for header changes, `Scripts/editor_tools/rebuild_restart.sh`) → wire it in the editor (assets, PerfTest placement, settings) → test in PIE → commit + push fixes → log in Docs/PROJE_GUNLUGU.md. The user also asked to keep GitHub up to date with everything done.

**Why:** the user saves Claude usage by moving code writing elsewhere; Claude does the editor-side work ([[reference-editor-mcp]]).

**How to apply:** always pull before working and push after; never force-push; respect Docs/GELISTIRME_AKISI.md (rules: [[project-assembly-not-modeling]], 75 cm module, existing systems, no third-party kits or >100 MB files in the repo — kits stay local, see Docs/RAPOR_2026-10-20.md). Editor helper tools now live in Scripts/editor_tools (repo-relative paths; set CMDREF to the Cmd box ref found by find_cmdref.py).
