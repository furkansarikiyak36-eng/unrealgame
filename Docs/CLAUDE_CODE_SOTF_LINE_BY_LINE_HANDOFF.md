# Claude Code Handoff — SotF Mechanics Branch (Authoritative Code Map)

**Branch:** feature/sotf-mechanics-expansion  
**Current source head when this report was written:** 72e26bdaf41f31b440d5d1b214ac96cfe66eda18  
**Target:** Unreal Engine 5.8 C++ plus engine-neutral C++17 mechanics modules  
**Execution policy requested by the user:** Do not run tests/builds in the current work. Tests in this document are source-only and are not reported as passing. Do not commit to or merge into main.  
**Read order:** Start with Docs/CLAUDE_CODE_START_HERE.md. Use Docs/CLAUDE_CODE_SOTF_CHANGE_MANIFEST.md for the file/commit index. This document is the detailed code map. Do not read unrelated sections when implementing one mechanic.

## 0. State and vocabulary

This branch contains two related, but not interchangeable, code layers:
- **Unreal runtime:** Actors/components and real world queries under Source/MyProject2 and Plugins/WorldSnap. These execute in the game world but have not been built or PIE-verified in this handoff.
- **Portable/reference mechanics:** deterministic C++17 rules under Modules. These are compiled through the Modules CMake project when enabled, but are not automatically linked into Unreal's MyProject2 module. Portable source and UE source need a deliberate adapter/parity decision.

Terms used:
- **Preview** = candidate result only; no spawn, resource debit or graph mutation is implied.
- **Commit** = final validation plus resource/actor/support graph changes.
- **Revision** = stale-evidence protection; callers must pass live revisions and refresh preview when evidence changes.
- **Snapshot / SaveRecord** = in-memory typed data contract, not proof of disk serialization.
- **Test source** = assertions/automation declarations exist. It does not imply build or execution.

## 1. Token-saving working instructions for Claude Code

1. Read START_HERE first, then read only the code-map section for the workstream being changed.
2. Use the exact path and line ranges below to navigate directly. Do not dump or reread all 5,000+ lines of ForestCharacter.cpp or the whole repo merely to understand prior work.
3. If the task touches code outside the relevant ranges, read only that target function and its directly called functions. Search for callers only when changing a public contract.
4. Before editing a source file, fetch its current branch version and current blob SHA. Line numbers are navigation hints and may change after edits.
5. After each source commit, update this handoff's source range and the Manifest entry for that file. Do not leave stale line numbers. If the change alters ownership/call flow, update the authority matrix.
6. Keep one authoritative state owner per mechanic. Do not drive the same fireplace/trap simultaneously from portable state and a separate Unreal state machine.
7. Do not declare tests/builds passing. Current work must not run tests unless the user explicitly changes that instruction.
8. Preserve main. Commit only to feature/sotf-mechanics-expansion.
9. For code edits, state: changed files, exact behavior, source/test line ranges, commit SHA, tests not run, remaining UE-only integration.
10. Do not copy long source files into a prompt. A path + relevant line range + the contract summarized here is the compact handoff.

## 2. Task router — read only the matching section

| Next task | Start here | Read these exact ranges |
|---|---|---|
| Stone-ring geometry (portable) | local_stone_ring_recognition.hpp/.cpp | header 8–35; cpp 11–82 |
| Actual player-placed stone hearth | StonePlacementComponent / StoneShapeRecognitionSubsystem / Campfire | placement 394–589 for graph connect/validation, 591–656 for commit/undo; recognition cpp 13–121 and 140–223; Campfire cpp 79–105 and 211–234 |
| Portable fireplace fuel/heat/save | stone_fireplace_lifecycle.hpp/.cpp | header 10–89; cpp 12–74, 75–183, 184–234, 235–254 |
| UE trap overlap and payload | TrapActor + TrapMechanismComponent | TrapActor.cpp 27–39, 44–54, 59–109; Mechanism.cpp 40–102 |
| Portable trap rules/save | trap_lifecycle.hpp/.cpp | header 7–104; cpp 12–28, 29–148, 149–245, 246–259 |
| Generic Unreal snap | WorldSnapTypes + WorldSnapResolver | types 22–67; resolver 5–137; tests 8–198 |
| Two-end log solver | portable and UE two-anchor solver | portable hpp 12–88, cpp 12–250; UE h 9–95, cpp 14–164, tests 35–195; ForestCharacter.cpp 1747–1814 |
| Log jacking / temporary support | CarvableLogJackingComponent | cpp 24–76, 79–97, 98–171; Character controls near 1234–1305 |
| Mixed wood/stone support | StructureLink + StoneActor | StructureLink.cpp 13–119, 121–222, 254–307, 382–427, 487–573; StoneActor.cpp 35–81 |
| Portable structural graph | structural_integrity.hpp/.cpp | header 6–67; cpp 8–206, 208–269 |
| Stakes / wattle / clay infill | StickPhaseS5_Defense | StakeActor.cpp 19–50, 73–153; StakeWallComponent.cpp 14–77; StakeReinforcementComponent.cpp 12–36; WattleFenceComponent.cpp 18–167 |
| Raw-log cutting/splitting/shape conversion | RawLogShapes.cpp | 56–124, 188–304 |
| Test source only | module_tests.cpp / WorldSnap Automation | portable test map in section 3.7; UE solver automation in section 4.2 |

The “386–?” and “590–660” ranges in the stone placement row are intentionally marked as recheck points: the source map below describes the exact function names, but confirm current line number with a narrow function search before referencing those two ranges in a new change.

## 3. Portable C++ modules

### 3.1 Local stone-ring geometry recognition

Files:
- Modules/GameplayIntegration/include/world_mechanics/local_stone_ring_recognition.hpp — 39 lines; contract SHA 1cc14a7e8b93ec7021ac051bacbed17e59f31e32.
- Modules/GameplayIntegration/src/local_stone_ring_recognition.cpp — 84 lines; implementation SHA 3e0e1859f9c6c8e7ad5f3e2ef3d12ed78b7b0177.

Header map:
- **8–11:** LocalStoneSample = stable ID, XYZ in portable world units/meters, active flag.
- **13–21:** StoneRingConfig: min/max count, radius band, radius deviation, height-span, largest angular gap, center-clearance radius.
- **23–30:** StoneRingAssessment output: accepted/rejected, contributing count and measurements/reason.
- **32 onward:** LocalStoneRingRecognizer::Assess is a pure assessment API. It does not spawn a fireplace.

Implementation map:
- **11–27:** Validate finite center/config and reject invalid settings.
- **28–43:** Deduplicate/collect active candidate stones; compute radius/angle; any active sample inside the center clearance radius currently rejects the ring before height/ring-band filtering. This is deliberately strict but can reject an unrelated vertical-layer stone; improve by adding semantic/material and bounded-local-query filtering before this function.
- **44–53:** Enforce min/max candidate count and sort candidates by angle.
- **55–69:** Compute mean radius, average absolute radius deviation, height span and largest angular gap.
- **70–80:** Reject inconsistent radius, height or open ring; otherwise set accepted result and reason.
- **81–84:** Function/namespace close.

Runtime distinction:
- This portable recognizer is **not** the actual UE player placement call path.
- Actual UE recognition is UStoneShapeRecognitionSubsystem::TryRecognizeManualFireplace after a successful placed-stone commit; see section 4.4.
- The portable recognizer's current all-active-sample center rejection and the UE subsystem's world-wide TActorIterator collection are both known places to tighten local/semantic filtering. Do not claim bounded world-query cost simply because the portable input vector has a cap.

### 3.2 Portable stone fireplace lifecycle and safe restore

Files:
- Modules/GameplayIntegration/include/world_mechanics/stone_fireplace_lifecycle.hpp — 92 lines; blob e34734cbcb705d8f6d4bddc0e88d5cc5b323be4b.
- Modules/GameplayIntegration/src/stone_fireplace_lifecycle.cpp — 257 lines; blob b4e8c58dfb46adb21921d33df80995f114361ba5.

Header:
- **10–14:** NotBuilt / Ready / Burning states.
- **16–23:** Config limits ring geometry, fuel capacity/rate, heat, max time step and sample count.
- **25–43:** Runtime snapshot and time-step result. These are data outputs only.
- **45–51:** StoneFireplaceSaveRecord schema v1: saved revision, fuel, cooking-pot flag and wasBurning intent.
- **53–55:** Explicit engine boundary: no Actors, traces, inventory, lights or particles.
- **59–71:** Ring observation, fuel/ignite/extinguish/pot/time APIs and typed SaveRecord restore.
- **73–88:** Snapshot/revision and internal state. Line **87** records pending “resume burning after live ring revalidation” intent.

Implementation:
- **12–36 ConfigValid:** Config must be finite; center clearance must be below ring minimum radius.
- **38–74 ReassessRing:** Rejects invalid center/revision and over-budget local samples; rejects stale/equal source revision; runs ring recognizer; commits observation revision. If geometry is lost, state becomes NotBuilt and pending-resume intent is cleared. After restore, a previous fire resumes only after a newer live geometry observation accepts the ring.
- **75–95 AddFuel:** Only a recognized ring can be fueled; accepts only available capacity; stale state revision rejects.
- **96–116 Ignite:** Requires recognized ring and positive fuel. Already burning is a no-op, not a duplicate transition.
- **117–128 Extinguish:** Moves to Ready/NotBuilt and retains residual fuel.
- **129–143 PlaceCookingPot:** Requires a burning, recognized fireplace; cannot add duplicates.
- **144–183 Advance:** Bounded deterministic fuel burn. Rejects invalid or overlarge delta and stale revision. On fuel depletion, clamps fuel to zero and stops heat.
- **184–201 CreateSaveRecord:** Rejects internally inconsistent state; captures current fuel/pot/fire intent.
- **202–234 RestoreFromSaveRecord:** Validate schema/revision/fuel/overflow before mutation. Restores fuel and pot, but intentionally clears live ring evidence and heat; if the save was burning, keep a pending intent until ring revalidation.
- **235–247 Snapshot; 248–254 CurrentHeatOutput:** Snapshot is read-only. Heat is zero unless Burning and ring-recognized; otherwise nominal heat scales linearly with remaining fuel fraction.
- **256 onward:** Namespace close.

Known integration gap:
- The portable lifecycle is **not wired** into Source/MyProject2/StonePhaseE_Functional/StoneFireplace.cpp / AStoneFireplace. That Actor has its own CurrentFuel/bIsLit/tick state. Do not connect both as independent authorities; choose one state owner or create a single adapter. No UE SaveGame serializer currently maps StoneFireplaceSaveRecord to disk.

### 3.3 Portable trap lifecycle and safe restore

Files:
- Modules/GameplayIntegration/include/world_mechanics/trap_lifecycle.hpp — 108 lines; blob 79adcf70756e5533e8e3fb0710e14cc68111b521.
- Modules/GameplayIntegration/src/trap_lifecycle.cpp — 261 lines; blob 25d0d42e93d1d2cf06afdf72b6783e99c4d23930.

Header map:
- **7–19:** Trigger source and Disarmed/Armed/CoolingDown/Consumed states.
- **21–29:** Stable trap ID, accepted source, cooldown/minimum rearm, damage, one-shot and cooldown caps.
- **31–41:** Runtime snapshot, including hasTriggered independently of timestamp.
- **43–54:** SaveRecord schema v1. Stores secondsSinceLastTrigger and remainingCooldownSeconds, not a world absolute timestamp.
- **56–74:** Request/decision. Decision returns side-effect instructions but does not apply damage/physics/audio.
- **78–91:** Public lifecycle API incl. CreateSaveRecord/RestoreFromSaveRecord.
- **93–104:** Config + authoritative state/history fields.

Implementation:
- **12–28 ConfigValid:** Trap ID, enum, finite cooldown/damage and caps.
- **29–49 Arm:** Revision gate; one-shot cannot rearm; cooldown cannot be bypassed by manually arming; already armed is idempotent.
- **50–67 Disarm:** Config/revision checks; one-shot cannot be disarmed as if reusable.
- **68–129 TryTrigger:** Config/event/time/revision/source/state checks; monotonic event ID prevents duplicate/out-of-order events; time cannot go backward. Effective cooldown is at least minimumRearmDelaySeconds even when configured cooldown is zero. Accepted event advances state/revision and returns a decision.
- **130–148 Advance:** Only exits cooldown when deadline reached; stale time/revision rejected.
- **149–181 CreateSaveRecord:** Re-bases trigger history into elapsed duration and remaining cooldown.
- **182–245 RestoreFromSaveRecord:** Verifies schema, matching trap ID, valid state/history, lifecycle policy and bounded times. Absolute clock is rebased to newWorldTimeSeconds; revision advances so old requests fail.
- **246–259 Snapshot:** Runtime snapshot only, not disk serialization.

Known limitations:
- SaveRecord does not include a full hash of every tuning field (damage/trigger source/etc.); it primarily validates trap identity and one-shot/lifecycle compatibility. If designer configuration changes across save versions, add a config fingerprint or explicit migration policy.
- This portable model is not wired into UTrapMechanismComponent. Unreal trap runtime still has its own cooldown timer/state. Do not keep two independent authorities in parallel.

### 3.4 Generic portable snap resolver

File: Modules/PlacementSnap/include/world_mechanics/placement_snap.hpp — 114 lines; blob 95bb476d9f4b479b3019a29a85529dd1d9691926.

- **7–20 SnapCandidate:** Candidate identity/transform/normal/distance/socket/region/overlap/revision plus targetCompatible, supportValid and socketOccupied.
- **21–31 SnapRequest:** Query requirements, candidate budget and requireExplicitSocket.
- **36–54 SnapPlankPositionToGrid:** Metric grid helper for planks only. A 0.20 m pitch is an explicit request/configuration, not a universal log/stone/terrain grid.
- **55–88 Resolve:** Validates request and candidate budget; filters invalid geometry, region, collision, target compatibility, support state, occupied sockets, required explicit sockets and stale surface/classification evidence. Deterministic ordering prioritizes explicit socket, then distance and stable candidate ID.
- **89–111 ValidateCommit:** Rechecks current surface/classification revisions plus region/collision/target/socket/support validity. The UE provider must supply live values. Leaving new booleans at default true means the adapter has not actually wired the gate.

### 3.5 Portable two-anchor placement solver

Files:
- Modules/PlacementSnap/include/world_mechanics/world_snap_two_anchor_solver.hpp — 91 lines; current blob 11752eadabc7f7997ba097b810372dd1123a98e7.
- Modules/PlacementSnap/src/world_snap_two_anchor_solver.cpp — 251 lines; current blob d9a6638b9e1cefebf27ee4c4379b79454c58e523.

Contract:
- **hpp 12–25:** TwoAnchorSnapAnchor: stable anchor/owner IDs, position in meters, axis, surface/classification/support revisions, load capacity, availability/compatibility/support/socket occupancy.
- **hpp 27–47:** Request: local X endpoint offsets (6 m log uses -3/+3 m), length error, penetration and gap tolerances, pitch band, axis alignment, required load, desired center, expected revisions and candidate/pair budgets.
- **hpp 49–71:** Decision includes selected anchor IDs, solved transform/ends, length error, yaw/pitch, center score, capacity and source revisions.
- **hpp 74–88:** Resolve + ValidateCommit public contract.

Implementation:
- **12–31 IsFiniteRequest:** Rejects malformed dimensions/tolerances/pitch/revision/budgets.
- **33–46 IsEligible:** Requires unique/nonzero ID, finite location/axis, availability, type compatibility, load-bearing support, free socket, adequate capacity and matching revisions.
- **48–120 EvaluatePair:** Checks unique anchor IDs, endpoint distance against piece length, one-sided gap vs compression/penetration allowances, anchor-axis alignment and pitch. Derives transform origin from endpoint A's local offset; does not use camera pitch.
- **122–132 Better:** Deterministic ranking: optional center distance, absolute length error, anchor A ID, anchor B ID.
- **134–192 Resolve:** Caps candidate count and pair-evaluation count. Rejects repeated/zero IDs. Sorts eligible anchors by stable ID, evaluates ordered pairs, applies deterministic tie break; returns NotFound or BudgetExceeded explicitly.
- **194–249 ValidateCommit:** Requires collision-free volume, same surface/classification/support revisions, matching piece/preview IDs and both live endpoints. Re-resolves the pair and rejects if anchor IDs/transform/forward changed after preview.

Boundaries:
- Portable coordinates are meters; UE coordinates are centimeters. Explicit conversion is mandatory.
- Portable Transform only models position + forward. It does not encode full roll/asset-specific orientation; Unreal builds a full rotation in its own solver.
- The portable solver is not linked to Unreal ForestCharacter automatically. It is a reference implementation until a bridge is deliberately authored.
- Candidate/pair loops are capped (default 128 anchors and 16,384 pair evaluations) but the current ordered search may assess both A/B orientations; keep limits and deterministic tie behavior in mind.

### 3.6 Portable structural support graph and snapshot restore

Files:
- Modules/StructuralIntegrity/include/world_mechanics/structural_integrity.hpp — 69 lines; blob b23ab4c0268edaa4a31be1abdb4bc4d0d685480b.
- Modules/StructuralIntegrity/src/structural_integrity.cpp — 271 lines; blob b09daca13c23f8139511fa33798eb89e75f06d72.

Header map:
- **6–15 StructureNode:** ID, grounded/active flags, demand, capacity, health, collapse latch.
- **16–22 SupportEdge:** directed supporter→dependent edge, capacity, active state.
- **23–35:** SupportState, node report and graph assessment/revision.
- **37–42:** StructuralGraphSnapshot schema, saved revision, nodes and edges.
- **44–67:** Add/remove/update/damage/repair/collapse/Assess/snapshot API. Mutation APIs bump graph revision when state actually changes.

Implementation:
- **8–29:** AddNode/AddEdge/RemoveNode validation and registration/removal of linked edges.
- **31–75:** RemoveEdge, SetEdgeActive, SetNodeDemand, SetNodeGrounded and SetNodeCapacity. Reject invalid/missing IDs; no-op when value is unchanged; successful state changes increase revision.
- **77–95:** Damage, repair and idempotent collapse latch. Collapse-latched nodes require replacement rather than ordinary repair.
- **97–206 Assess:** Builds active dependency graph, sorts edge lists by ID for deterministic load distribution, uses stable topological order, rejects active cycles, propagates aggregate dependent loads upstream in equal shares, evaluates effective capacity by health, then reports Inactive/Unsupported/Overloaded/Supported. It is a deterministic gameplay approximation, not an FEA/Chaos solver.
- **208–229 CreateSnapshot:** Enforces node/edge limits, outputs sorted-by-ID snapshot.
- **231–269 RestoreSnapshot:** Validates schema/revision/limits; constructs a private candidate graph; validates IDs, references, endpoints and topology with Assess; only then replaces live node/edge maps and advances revision. Invalid snapshot must not partially change live state.

Critical architectural boundary:
- This portable StructuralGraph is not the same implementation as UE StructureLink/WoodStructure. UE already evaluates placed wood and stone through StructureLink; portable graph still needs an explicit adapter decision before claims of a shared authoritative runtime graph.

### 3.7 Portable build/test source map (tests not run)

- Modules/CMakeLists.txt — 51 lines. Portable source registrations: line 18 stone fireplace lifecycle; line 19 trap lifecycle; line 20 world_snap_two_anchor_solver; line 21 onward existing portable modules. Build options and test executable are below.
- Modules/tests/module_tests.cpp — 1275 lines.
  - **240 onward TestSnapAndExcavation:** generic placement gates, occupied/incompatible/unsupported candidates and commit-time live checks; plank grid and excavation checks.
  - **367–463 TestStructuralIntegrity:** load, mutation, support edge removal/disable, grounded/capacity change, snapshot roundtrip and rejection cases for dangling edge/cycle.
  - **1002–1060 TestStoneFireplaceLifecycle:** ring/fuel/ignite/pot/heat and save/restore; restore is NotBuilt/zero heat until ring revalidation, then re-ignites only if saved as burning; corrupt fuel save rejected.
  - **1063–1119 TestTrapLifecycle:** source gate, duplicate event, cooldown and one-shot plus relative-cooldown save/restore and trigger-history tests.
  - **1122–1207 TestWorldSnapTwoAnchorSolver:** endpoint-derived 45° pitch, transform, live commit, collision/revision/geometry change, socket/support/capacity/length/pitch/duplicate-ID and budget cases.
  - **1209–1240 TestLocalStoneRingRecognition:** ring coverage, open ring, center obstruction, count and invalid config.
  - **1244–1275 main / test table:** 20 portable test groups are registered. This count is source registration only, not PASS results.
- Plugins/WorldSnap/Source/WorldSnap/Private/Tests/WorldSnapTwoAnchorSolverTests.cpp — 197 lines, compiled only with WITH_DEV_AUTOMATION_TESTS. **35–137** prior tests: level, slope, length mismatch, non-load-bearing, same anchor and pitch. **139–167** new readiness/capacity cases. **170–195** compression/gap tolerance. These UE Automation tests have not run.
- Plugins/WorldSnap/Source/WorldSnap/Private/Tests/WorldSnapResolverTests.cpp — 199 lines; generic nearest/tie/group/occupied/unavailable/load-bearing assertions, source only.

## 4. Unreal runtime paths

### 4.1 Generic WorldSnap plugin

Files:
- Plugins/WorldSnap/Source/WorldSnap/Public/WorldSnapTypes.h — 81 lines, current blob 45ca9c43aea7d17aeb4373000dadad160ea38d73.
- Plugins/WorldSnap/Source/WorldSnap/Private/WorldSnapResolver.cpp — 138 lines, blob f317c65e29ddacfb79730ce04b4d017d01720227.
- Plugins/WorldSnap/Source/WorldSnap/Private/Tests/WorldSnapResolverTests.cpp — 199 lines.

Types:
- **Types.h 22–41 FPoint:** stable name/group, position/normal/axis, source index, available/occupied/load-bearing and semantic type.
- **Types.h 43–67 FQuery:** hit point/normal, max distance, group, normal/axis constraints, weighting and reject flags.
- **Resolver.cpp 5 onward:** deterministic data-only filtering/scoring. Lines **41–60** reject unavailable/occupied/non-loadbearing and invalid/group candidates; **68–110** normal and axis scoring; **111–137** best result/status.
- Ties preserve input order; provider must produce candidates in stable order. This resolver does not create actors, query the world, or commit inventory.

### 4.2 Unreal two-anchor solver

Files:
- Plugins/WorldSnap/Source/WorldSnap/Public/WorldSnapTwoAnchorSolver.h — 95 lines; current blob 42d0a53b76003b8e8cddda52eaed173fcbf97c06.
- Plugins/WorldSnap/Source/WorldSnap/Private/WorldSnapTwoAnchorSolver.cpp — 165 lines; blob a5bcc9863d0c1c2bce22c0e584d8b4e197f6a9e5.
- Automation source above.

Header:
- **9–32:** Anchor kinds and result/rejection statuses incl. occupied, incompatible and invalid support.
- **38–51:** Anchor fields: ID/kind/position/normal, availability/load-bearing, occupied, target-compatible, support-valid and max capacity.
- **53–72:** Query and result. Query includes piece length/local X endpoints, allowed length error, gap/penetration, required load and pitch.
- **90–95:** Solver static API.

Implementation:
- **14–22 IsValidAnchor:** Valid kind/finite point/normal and finite nonnegative load capacity.
- **25–53 Solve input validation:** Reset result, validate length, local endpoint extent, tolerances, pitch and UpHint.
- **55–89 readiness checks:** valid anchors, available, unoccupied, target-compatible, load-bearing/support-valid and enough capacity; then enforce distinct anchor IDs.
- **91–128 geometry:** endpoint delta/distance, signed/absolute length errors, independent free-gap and compression/penetration gates, endpoint-derived pitch.
- **130–149 rotation/origin:** full Unreal quaternion derives from direction + UpHint; actor origin comes from local start endpoint.
- **151–164:** endpoint alignment tolerance verification then Resolved.
- New automation **139–195** tests occupancy, target compatibility, missing support, insufficient load, tolerated penetration and allowed/denied gap.

### 4.3 ForestCharacter two-anchor path

File: Source/MyProject2/ForestCharacter.cpp — current length 5284; blob ff482e8ca62a8192d9af1f304a9056283ca25046. Header current length 623.

- ForestCharacter.h **80–84:** player-facing log lifting/support methods.
- ForestCharacter.cpp **1234 onward:** LiftAimedLogOneEnd, SupportAimedLogWithCarriedPiece and related input/control flow; inspect only this range for jack controls.
- ForestCharacter.cpp **1518–1545:** maps CarvableLog snap points into generic WorldSnap FPoint candidates and uses FWorldSnapResolver.
- ForestCharacter.cpp **1683–1720 onward:** support preview cache/current candidate behavior.
- ForestCharacter.cpp **1747–1814:** sloped log case. Requires slope mode, unsplit piece and an upright post target. Computes post top and ground endpoint from geometry/length/radius; builds FTwoAnchorQuery with ground + post top; calls FWorldSnapTwoAnchorSolver::Solve; when valid sets preview transform, Slope mode, label and EvaluateSupportPreview.
- ForestCharacter.cpp **1790–1795:** target post support/occupancy + piece-length/local endpoint values. Post top gets bLoadBearing/bSupportValid from anchored state and bOccupied from FindBeamOnPost.
- This is currently a targeted ground-to-post slope path, not a generic arbitrary socket-pair provider for all construction modes. It does not use the portable solver and currently lacks the portable solver's explicit ValidateCommit API. Verify how the placement action recomputes the preview result before changing final commit.
- Do not reread all of ForestCharacter.cpp for a slope edit; start at the block above and follow only the direct functions you need.

### 4.4 Player-placed stone rings and fireplace Actor

Files:
- Source/MyProject2/StonePhaseF_Environment/StoneShapeRecognitionSubsystem.h — 61 lines.
- Source/MyProject2/StonePhaseF_Environment/StoneShapeRecognitionSubsystem.cpp — 223 lines.
- Source/MyProject2/StonePhaseB_Placement/StonePlacementComponent.cpp — 729 lines.
- Source/MyProject2/StonePhaseA_Mining/StoneActor.cpp — 115 lines.
- Source/MyProject2/Campfire.h/.cpp — 84 / 234 lines.

Recognition:
- StoneShapeRecognitionSubsystem.cpp **13–121 EvaluateHearthRing:** geometry/shape evidence calculation for UE stones.
- **122–139 IsHearthAlreadyPresent:** prevents duplicates near an existing campfire.
- **140–223 TryRecognizeManualFireplace:** verifies newly installed stone; gathers installed stones with TActorIterator; deterministic candidate ordering prioritizes the newly placed stone; bounds candidate count; checks ring; only then calls ACampfire::BuildWithExternalStoneRing.
- The current actor enumeration traverses all AStoneActor instances before filtering to local candidates. If optimizing this, preserve deterministic ordering and ring acceptance semantics, but use a bounded local spatial query/cell index rather than scanning every stone during each placement.

Placement:
- StonePlacementComponent.cpp **82 onward BeginPlacement:** validates carried/loose stone and transitions it into placement preview.
- **127 onward CancelPlacement:** restores prior state/transform and clears preview.
- **194 onward UpdatePlacementTarget / 251 onward ApplySelectedPlacementOption:** target face/socket choice, orientation and preview transform.
- **394–589 TryConnectStructuralTarget and supporting helpers:** checks structural target/graph/definitions, picks a matching source face/socket and finds a live unoccupied compatible target socket with aligned normal/position.
- **591–656 ConfirmPlacement / UndoLastPlacement:** final overlap clearance, transform, graph connect, Installed transition; rollback of a newly created connection if installation fails; only after installation succeeds releases carry reference, appends placement history, invalidates/rebuilds shared support, and calls TryRecognizeManualFireplace on the newly installed stone.
- **UndoLastPlacement:** disconnects graph edges before returning stone to Loose, then invalidates and settles support.
- TryConnectStructuralTarget begins at line 394; ConfirmPlacement at 591; UndoLastPlacement at 657; TickComponent at 694. Do not paste/read all 729 lines.

Actor:
- Campfire.cpp **79–105:** Build and BuildWithExternalStoneRing; external mode hides built-in ring mesh/collision to avoid double ring visuals.
- Campfire.cpp **around 180–234:** heat/fire tick and scorches nearby wood; see current source for exact internal function.
- StoneActor.cpp **35–81:** structural state, shape and damage; damage/fracture invalidates StructureLink and calls Settle.
- This Unreal path actually recognizes a placed stone ring. Portable LocalStoneRingRecognizer and StoneFireplaceLifecycle have not been wired into this Actor path.

### 4.5 Shared mixed wood/stone support

Files:
- Source/MyProject2/StructureLink.h — 86 lines; blob 5f983a4ec37ee367b66a7068218fa560cfed5e5a.
- Source/MyProject2/StructureLink.cpp — 573 lines; blob 4c721f461bb085439731c12d5dcdb354fb9ec431.
- Source/MyProject2/StonePhaseA_Mining/StoneActor.cpp — 115 lines.
- Main geometry rules are in Wood/WoodStructure* and Wood/WoodStructureGeometry*; do not re-read all files unless changing those contracts.

Code map:
- StructureLink.cpp **13–119:** Ground detection, gathering installed structural parts, mixed ACarvableLog + installed AStoneActor members and graph edge generation.
- **121–222 Evaluate:** wood span/overhang support rules; maximum support gap is 8 modules and maximum overhang is 2 modules; unsupported beams can be marked to snap unless placement grace applies.
- **224–252 MakeNode(AC arvableLog):** log endpoint geometry, length module clamp, notch/cross-section/end shape, role, grounded/damage/temporary state.
- **254 onward MakeNode(AStoneActor):** maps stone definition role/oriented bounds into same wood geometry graph.
- **382–427 Features:** caches by both world and StructureVersion, builds mixed material feature recognition, returns actor members plus legacy log-only list.
- **487–549 Settle:** bounded up to 16 passes; rebuilds graph after a failure; logs may snap/fall, unsupported stone returns to falling rigid-body state. This is UE runtime authority for mixed support.
- **556–573 WouldBeSupported:** adds a candidate log at proposed transform and assesses it against current mixed graph.
- StoneActor.cpp **66–81:** structural damage/fracture invalidates shared support and settles the world.
- Known limit: this is geometry/topology gameplay logic, not a physical material solver. Settle can scan all installed structural actors; avoid adding extra full graph rebuilds per Tick. Portable StructuralGraph is a separate implementation and is not wired to this actor system.

### 4.6 Log jacking / supporting a raised endpoint

File: Source/MyProject2/Phase2_Transport/CarvableLogJackingComponent.cpp — 177 lines; blob b737291044dd47908bd06aeabd801963f9aa9945. Header 54 lines.

- **24–76 LiftOneEnd:** validate a lying log and raise amount; moves one endpoint without treating the whole log as a rigid pure translation. Original transform/state should be retained for rollback.
- **79–97 GetEndpointGeometry:** calculates endpoint axis/position/radius used for support placement.
- **98–171 PlaceSupportUnder:** requires compatible stable log/support piece and module height; places the real support piece, asks StructureLink::WouldBeSupported, and rolls back support ownership/transform if graph rejects; on success settles/reassesses.
- **172–177 IsFullySupported:** delegates to StructureLink's actual support result.
- Character header **81–84** public lift/support controls. Character cpp around **1234–1305** invokes them and manages input/held item.
- This is an actual Unreal component flow; it is separate from portable TwoAnchorSnapSolver. Avoid representing the portable model as already wired to this component.

### 4.7 UE trap actors / trigger lifecycle / payloads

Files:
- Source/MyProject2/Phase6_Utility/TrapActor.h — 66 lines.
- Source/MyProject2/Phase6_Utility/TrapActor.cpp — 109 lines.
- Source/MyProject2/StickPhaseS6_Traps/TrapMechanismComponent.h — 72 lines.
- Source/MyProject2/StickPhaseS6_Traps/TrapMechanismComponent.cpp — 102 lines.
- Source/MyProject2/StickPhaseS6_Traps/SpringTrap.cpp — 76 lines.

TrapMechanismComponent.cpp:
- **40–45:** Trigger delegates to TriggerFrom with configured source.
- **45–90:** source/config/time/cooldown/state checks; apply damage; broadcast accepted trigger; transition to Consumed or Cooldown; zero-duration cooldown finishes next tick to block repeated overlaps in a single frame.
- **93–102:** cooldown completion only re-arms if still in cooldown.
TrapActor.cpp:
- **27–39:** validates tuning then passes damage/cooldown/consume policy to mechanism.
- **59–79:** overlap maps pressure/proximity overlap to triggers; tripwire/manual needs explicit signal.
- **81–99:** accepted trigger releases real payload log (LetFall) and optionally adds rolling impulse; spike pit launches a character. It does not fabricate a payload log when one is absent.
SpringTrap.cpp:
- **9–26 constructor:** creates BaseMesh as root before attaching child meshes/constraint; creates Mechanism.
- **28–36:** clamp finite spring tension.
- **38–63:** validate victim and a supported physical/character launch target before consuming mechanism trigger.
- **65–75:** after Mechanism->Trigger succeeds, applies LaunchCharacter or AddImpulse.
- UE TrapMechanismComponent is the live game authority today. Portable TrapLifecycle's event IDs/relative-clock save model is not wired to it; use one authority, not two.

### 4.8 Stakes, sharpening, stone reinforcement and wattle fences

Files:
- Source/MyProject2/StickPhaseS2_Processing/StickSharpeningComponent.cpp — 68 lines.
- Source/MyProject2/StickPhaseS5_Defense/StakeActor.h/.cpp — 71 / 153 lines.
- Source/MyProject2/StickPhaseS5_Defense/StakeReinforcementComponent.cpp — 36 lines.
- Source/MyProject2/StickPhaseS5_Defense/StakeWallComponent.cpp — 77 lines.
- Source/MyProject2/StickPhaseS5_Defense/WattleFenceComponent.cpp — 202 lines.

Mechanic map:
- Sharpening component **12 onward:** multi-hit sharpening progress; target shape changes only after enough hits. Look at GetRegisteredHits/GetSharpenProgress around **51–68** for state reporting.
- StakeActor.cpp **19–50:** collision/damage volume setup; **73 onward:** sharpened stake increases damage and enables damage overlap; **81 onward:** stone reinforcement validates count/tuning and attaches real stone mesh pieces.
- StakeReinforcementComponent.cpp **12–27:** validates unique, installed/carried physical stone references and consumes stones only if reinforcement succeeds. **28–36:** optional count-only route is configurable; do not treat it as the physical-resource path.
- StakeWallComponent.cpp **14–77:** validates wall endpoints/count; stages spawned stakes and destroys staged actors on failure; only swaps previous wall after all replacement stakes are placed.
- WattleFenceComponent.cpp **18–40 RollbackPieces:** disconnects graph relations before destroying newly staged pieces.
- **42–142 BuildFence:** input/cap limits, staged posts/rails, socket connections and full rollback on failure; old fence retained until new fence connects successfully, then old set is removed.
- **145–202 AddClayFill:** creates/registers infill component and tracks it by fence section. Verify component ownership/cleanup if adding fence save/restore.
- These are Unreal runtime systems; they do not yet share a complete persistence transaction adapter with portable persistence modules.

### 4.9 Raw log shape conversion transaction

File: Source/MyProject2/RawLogShapes.cpp — 327 lines; blob f3c4f1c550f3f2dc2ab5d72b5496e1a20b8478bd.

- **56–124 Outcome:** calculate proposed output shape/action before state mutation.
- **188–213 CanApply/Apply preflight:** validate tool/action/shape and determine outputs.
- **215–256:** explicitly preflight full result before mutating actor/resource state.
- **257–294:** disable source collision while staged output pieces spawn; if any spawn fails destroy all spawned outputs and restore source; only after complete spawn success commit source conversion, then settle/invalidate structure.
- **304 onward:** action/profile/output description for debugging/UI.
- This is the rollback pattern to reuse for build placements: preflight → stage actors → validate all → commit resource and graph changes; on any failure destroy staged actors and restore original state.

## 5. Existing planning documents (read rather than rediscover design)

- Docs/SOTF_MECHANICS_EXPANSION_PLAN.md: design intent and SOTF behavior clusters; references logs, free building, cut/split/planks, supports, openings, traps, rope/logistics and acceptance goals.
- Docs/SOTF_MECHANICS_IMPLEMENTATION_BACKLOG.md: workstreams A–H, candidate provider/commit rules, bounded local scans, support/collapse, survival/environment, persistence and quality gates.
- Docs/WorldSnapLinkedPackage.md: why WorldSnap is a provider-independent plugin and how Character log-face snap routes through shared resolver.
- Docs/KARARLAR.md: architectural decisions ledger; check before changing a locked design rule.
- Modules/PlacementSnap/SPEC.md (37 lines): responsibility, provider inputs, eight-step candidate pipeline, atomic spawn/inventory/graph contract. Two-anchor additions occupy **28–37**.

Do not ask the user to repeat decisions already stored in these docs. If a code behavior conflicts with a design rule, report the discrepancy and fix the narrowest source.

## 6. Single-owner matrix — avoid two competing implementations

| Behavior | Current UE owner | Portable/reference owner | Integration truth |
|---|---|---|---|
| Generic point snap | FWorldSnapResolver in Plugins/WorldSnap | PlacementSnapResolver | Same intent, separate code and data types |
| Slope kütük between ground and post | ForestCharacter + FWorldSnapTwoAnchorSolver | WorldSnapTwoAnchorSolver | UE path uses UE solver; portable solver is not linked |
| Wood + stone support | StructureLink/WoodStructure | StructuralGraph | Separate implementations; UE StructureLink is current world support authority |
| Manual stone hearth recognition | StonePlacementComponent + UStoneShapeRecognitionSubsystem + ACampfire | LocalStoneRingRecognizer + StoneFireplaceLifecycle | Portable side not wired to Actor |
| UE trap trigger/payload | UTrapMechanismComponent + ATrapActor/ASpringTrap | TrapLifecycle | Portable side is not wired; don't run two state owners |
| Persisted terrain density | existing DensitySnapshotCodec | density snapshot portable module | Terrain codec exists; not evidence that actor/save systems are serialized |
| Structure SaveRecord | no complete UE SaveGame binding documented here | StructuralGraphSnapshot typed in-memory model | No byte codec/UE binding yet |

## 7. Quality risks and honest current status

- **No tests/builds were run for this handoff.** No CMake/CTest command, UBT/UHT build, editor Automation test or PIE session has been executed here.
- There are portable test source cases and UE Automation source cases, but zero new pass claims are allowed.
- Portable typed records/snapshots are not yet byte codecs or complete Unreal SaveGame records. Disk save, migrations and crash recovery remain open.
- Portable fireplace model is not wired to AStoneFireplace. Portable trap model is not wired to UTrapMechanismComponent. Portable structural graph is not wired to StructureLink. Do not claim otherwise.
- Generic snap still needs full provider-driven world candidates, real overlap-volume checks and atomic actor + inventory + graph rollback.
- ForestCharacter's current two-anchor implementation is only a specific ground-to-upright-post slope scenario. General two-end beam/post/cross-log providers and final commit revalidation remain to be wired.
- Stone ring portable input is bounded, but UE recognition currently enumerates world stone actors. A future performance slice should create a bounded nearby-stone query or cell registry, preserving center/radius/gap filters.
- StructureLink's live graph includes installed logs and placed stones, but its evaluation is a deterministic geometric support approximation and can rebuild all installed parts. It is not FEA and not a substitute for Chaos.
- SaveRecord restores validate basic lifecycle/identity; configuration fingerprints/migrations still need explicit policy, particularly for traps.
- No performance profiling exists. Do not claim CPU budget or performance gains without measured evidence.

## 8. Ordered continuation checklist

1. Bring this report and the manifest up to date after every code commit.
2. For two-anchor placement, add/finish UE provider data for both endpoints (stable ID, occupancy, target compatibility, support/load and source revisions), then revalidate both anchors and collision in the exact final placement/commit flow.
3. Connect actual fireplace lifecycle to one UE owner, using bounded stone query and one state authority; make inventory/fuel transition rollback-safe.
4. Decide whether portable TrapLifecycle should become the UE mechanism authority or remain a specification/reference model. If integrated, map event ID and time/revision carefully; do not run both state machines independently.
5. Decide if portable StructuralGraph should remain a reference model or adapter to UE StructureLink. Do not maintain duplicate live graphs without explicit synchronization.
6. Add persistence byte codecs/UE SaveGame adapters and migration/version policy for Actor IDs, placed stones, traps, fireplace, support graph, inventory, terrain edits and carrying/transient object policies.
7. Only when the user authorizes tests, run in order: portable CMake/CTest; Unreal 5.8 UBT/UHT; WorldSnap/other Editor Automation; PIE physical placement/collapse/restore cases. Report each independently.
8. Continue toward remaining SOTF-style mechanics: log end carving and split-to-planks, wall openings/gates, foundations/floors/roofs/ramps, physical payload traps, stakes/defensive walls, storage/transport ownership, farm/soil/water/season interactions and global environment events. Reuse the scan/provider and staged commit boundaries; don't create isolated one-off snap algorithms.

## 9. Source commit anchors for the latest code slices

- Portable hearth recognizer: 655998aa4d5cab0bf5342b3b379ccc70f6d90de8, 3e07f468f399ed63837707eb7ac8a9c155b38cc8, 623debc9ad9e4247b566f7299e556e4a98dc7e07, 05fb4a23b50a8fde8ce1ce3dc3b280f6889629dc, 6cae18ae32cfb2b486977a79c9fdfdde436d10a8.
- Portable fireplace/trap lifecycle and source test cases: a6db887ae8d49b1be4e63a972890ee4132055418 through fa463137c3475d17b2725c027cdb38bff05ec4ab; use branch history for individual commits.
- Structural graph mutations/snapshot: 0e77346c4ee1c5e1a4c7e18c337c8f8159ad2e3d, c7ba8081976052df4ab31825fd291a05420935be, b84b0903152946e95821798b7ea4ee49f15116fd, e9b04928f2ea075a44f0e4427b7cb94465a18d33, 9627be9c429db68a3a5035f80bfe952e8c47db5b.
- Portable two-anchor solver: d313ce7a132bb22db03cbc5a12a056bd22be74af, 7c73016cdc8ecb69aaa00ce304c005854b6f955f, 7f602b8f7fb6029cd7d7f57c2a257c6688cdad3d, 81a7a2c0c5cbf6d655eae2648be6883e5bdae257, e912a50e6ad843ef3bb2c34b1f7669f43cb4205c.
- Unreal two-anchor gates/automation + Character bridge: 1f7ca07332992cde839b8fd4b106778ad193eddd, 2b9444ef56cd4e28e418e20ce8b35608e262c6a4, d39de8e9735defdd328c5286d4ef45ff28e77aef, 24a79bf3f1e1ec6389397b06de3fcfdbdd6b163b, 72e26bdaf41f31b440d5d1b214ac96cfe66eda18.
- This report must be paired with Docs/CLAUDE_CODE_START_HERE.md and Docs/CLAUDE_CODE_SOTF_CHANGE_MANIFEST.md for the lowest-token continuation path.
