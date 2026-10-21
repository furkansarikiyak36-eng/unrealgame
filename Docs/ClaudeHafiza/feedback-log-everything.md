---
name: feedback-log-everything
description: "After every task, append an entry to Docs/PROJE_GUNLUGU.md and refresh generated docs so web Claude can read cheaply"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 9f227c4e-3bbf-4c74-a5be-b25df61a6efb
  modified: 2026-10-13T10:03:30.988Z
---

The user wants everything done from now on recorded in `Docs/` so web Claude (limited tokens, can't read .cpp) can catch up from .md files.

**Why:** web Claude burns tokens quickly; reading docs is far cheaper than reading source. The user said "bundan sonra yapılan kaydedilen herşey kaydedilir".

**How to apply:** after each task (1) append an entry to `Docs/PROJE_GUNLUGU.md` (append-only: what, files, test result, bugs, remaining), (2) update `Docs/KARARLAR.md` / `Docs/BASLA.md` / `Docs/DURUM_RAPORU.md` if decisions or state changed, (3) run `python Scripts/update_docs.py` (or `bash Scripts/run_tests.sh` with editor closed). Tunnel stays read-only; web Claude delivers zips. See [[project-greybox-rebuild]].
