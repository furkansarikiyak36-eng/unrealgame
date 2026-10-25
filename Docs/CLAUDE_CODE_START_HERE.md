# Claude Code — START HERE (Low-Token Protocol)

**Repository:** furkansarikiyak36-eng/unrealgame  
**Only working branch:** feature/sotf-mechanics-expansion  
**User instruction:** do not run tests/builds in the current work unless the user explicitly authorizes it. Never modify or merge into main.

## Do this before any code task

1. Read this file only.
2. For a new coding session, copy the instructions from **Docs/CLAUDE_CODE_NEXT_WORK_PROMPT_TR.md** into Claude Code. That prompt fixes the first source files, acceptance gates and final report format. Treat it as the execution brief, not as a request for repo-wide analysis.
3. Open **Docs/CLAUDE_CODE_SOTF_CHANGE_MANIFEST.md** and locate the one mechanic/file group for the task.
4. Open **Docs/CLAUDE_CODE_SOTF_LINE_BY_LINE_HANDOFF.md** and read only the matching section/range.
5. Fetch only the targeted source file and the specific line range/functions that must be changed. Do not reread the repo, huge source files or unrelated systems.
6. Make the smallest coherent implementation on this branch, record the commit SHA, then update the relevant handoff/manifest entry with new line numbers and remaining integration work.
7. Do not run CMake, CTest, Unreal Build Tool, UHT, Automation or PIE in the current stage. Test source is not test success.

## What is currently implemented

- Portable C++: local stone-ring geometry recognition; fireplace lifecycle + typed save/restore record; trap lifecycle + event dedupe + relative-cooldown save/restore; generic snap candidate resolution; two-anchor placement resolution/commit validation; structural support graph mutation and typed snapshot restore.
- Unreal C++: generic WorldSnap plugin; Unreal two-anchor solver with occupancy/compatibility/support/capacity and gap/penetration rules; the specific ForestCharacter ground-to-upright-post slope placement calls the UE solver; player-placed stone hearth recognition after successful stone installation; mixed wood/stone support through StructureLink; log jacking and real support-piece validation; trap trigger/payload/spring trap gating; transactional stake walls, wattle fences and raw-log shape conversion.
- The branch contains code from earlier mechanics slices too. The Manifest maps those files and the detailed handoff explains their call paths.

## Critical ownership boundaries

| Mechanic | Current runtime authority | Portable/reference code |
|---|---|---|
| UE generic point snapping | Plugins/WorldSnap FWorldSnapResolver | Modules/PlacementSnap PlacementSnapResolver |
| UE sloped log | ForestCharacter + Plugins/WorldSnap FWorldSnapTwoAnchorSolver | Modules/PlacementSnap WorldSnapTwoAnchorSolver |
| Mixed wood/stone support | Source/MyProject2/StructureLink + WoodStructure | Modules/StructuralIntegrity StructuralGraph |
| Actual player-placed hearth | StonePlacementComponent + StoneShapeRecognitionSubsystem + Campfire | LocalStoneRingRecognizer + StoneFireplaceLifecycle |
| Actual UE trap | TrapMechanismComponent + TrapActor/SpringTrap | TrapLifecycle |

Portable and Unreal versions are **not automatically wired together**. Do not describe them as one shared runtime system or run two independent state authorities for one mechanic. The engine-neutral records are typed in-memory contracts, not proof of disk SaveGame serialization.

## Pick the relevant section

- Snap, kütük endpoint, angle, support, preview/commit: read Manifest section **B**, then handoff sections **4.1–4.3** and **3.4–3.5**.
- Stone ring / fireplace: Manifest section **C**, then handoff **3.1–3.2** and **4.4**.
- Mixed material support / collapse: Manifest section **C**, then handoff **3.6** and **4.5**.
- Log jack / temporary support: Manifest section **B**, then handoff **4.6**.
- Trap lifecycle / physical payload / spring launch: Manifest section **D**, then handoff **3.3** and **4.7**.
- Stake, fence, clay infill: Manifest section **D**, then handoff **4.8**.
- Raw log split/trim/hewing transaction: Manifest section **E**, then handoff **4.9**.
- Portable regression source: handoff **3.7** only. Do not execute it without user authorization.

## Last verified source references

- Source implementation commit before this documentation packaging: **72e26bdaf41f31b440d5d1b214ac96cfe66eda18**.
- Detailed handoff was replaced and corrected in **7972535ca5b79713ed95a1538f28e640e3bdb518**, then exact StonePlacement ranges were pinned in **0fff11df3a22a0ad4996deacc144d2cb5074c5ac**.
- Manifest commit: **da335ff685f47e91763f15046de667765e286be4**.
- Reusable Turkish execution prompt commit: **dca388debf8ddda732a92d6b6fa4fe8d62c141c9**.
- These are source/document commits only. The test/build state is: **not executed**.

## Required report for every future code change

Record in the same task section:
- file + function/line range changed;
- behavior/algorithm changed, inputs and outputs;
- state authority and actual caller;
- error/rollback/revision edge cases;
- test source added or updated (not run);
- commit SHA;
- Unreal-only integration still missing;
- new exact line numbers.

Avoid full-file pastes, repository-wide scans and duplicate design explanations. The detailed map is intentionally the cheaper substitute for a fresh full-code analysis, but still inspect exact target lines before editing them.
