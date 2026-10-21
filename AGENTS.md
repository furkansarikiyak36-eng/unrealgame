# For AI agents working on this repository

Start with `Docs/DEVIR_TESLIM.md` (state, build and test, pitfalls), `Docs/DEPO_YAPISI.md` (what is where), then `Docs/BASLA.md` and `Docs/GELISTIRME_AKISI.md`. The user speaks Turkish; answer in Turkish.

## Who does what (from 2026-10-21)
- **ChatGPT (web, reads GitHub directly): C++ code.** Writes mechanics in `Source/MyProject2` on its own branch `chatgpt/<topic>`, never directly on `integrate/web-plan`. It cannot compile or run Unreal: write clean code, reuse existing functions (`Docs/KOD_HARITASI.md`), document new public functions, and say in the commit message how to test it.
- **Claude Code (on the user's PC, Unreal open): everything in the editor and anything that needs detail.** Pulls the branch, builds, fixes compile errors, wires it in the editor (assets, maps, settings), tests in play, bakes showcases, merges into `integrate/web-plan`, pushes, writes the journal.
- Codex is not used any more.

## Rules
- Unreal Engine 5.8, C++ module `Source/MyProject2`. Working branch `integrate/web-plan`; never force-push.
- Third-party asset kits are not in the repository (licence and size); they live on the user's disk.
- After every task: append an entry to `Docs/PROJE_GUNLUGU.md`, run `python Scripts/update_docs.py`, commit and push.
- Do not save maps or delete content without the user: two tools must not drive the editor at the same time.
