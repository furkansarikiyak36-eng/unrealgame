# SotF Mechanics Change Manifest — Claude Code Index

**Branch:** feature/sotf-mechanics-expansion  
**Current task policy:** no tests/builds are to be run until the user explicitly changes this instruction; no changes to main.  
**Use this file for:** quickly locating what changed, where the behavior lives, and which file owns the real runtime. For behavioral detail use Docs/CLAUDE_CODE_SOTF_LINE_BY_LINE_HANDOFF.md. For initial instructions use Docs/CLAUDE_CODE_START_HERE.md.

## 1. File inventory by mechanic

### A. Portable engine-neutral code

| File | Current size | Responsibility / changed behavior | Read only when |
|---|---:|---|---|
| Modules/GameplayIntegration/include/world_mechanics/local_stone_ring_recognition.hpp | 39 lines | Stone sample/config/assessment contract | Changing ring parameters or result schema |
| Modules/GameplayIntegration/src/local_stone_ring_recognition.cpp | 84 lines | Bounded input geometry assessment: unique active stones, center-clearance, radius band, radius variance, height and angular coverage | Changing ring recognition rules |
| Modules/GameplayIntegration/include/world_mechanics/stone_fireplace_lifecycle.hpp | 92 lines | Fireplace states, config, snapshot, schema-v1 save record and resume-after-ring-validation flag | Changing fuel/heat/pot/save contract |
| Modules/GameplayIntegration/src/stone_fireplace_lifecycle.cpp | 257 lines | Ring source revision, fuel/ignite/extinguish/pot/time transitions, safe typed restore | Changing fireplace state transitions |
| Modules/GameplayIntegration/include/world_mechanics/trap_lifecycle.hpp | 108 lines | Trap state, request/decision, event history and relative cooldown save record | Changing trigger state/schema |
| Modules/GameplayIntegration/src/trap_lifecycle.cpp | 261 lines | Source and event-ID gates, cooldown, one-shot state, relative-clock restore | Changing trap lifecycle or save rules |
| Modules/PlacementSnap/include/world_mechanics/placement_snap.hpp | 114 lines | Generic candidate filtering/ranking and commit-time snap validation | Changing portable generic snap |
| Modules/PlacementSnap/include/world_mechanics/world_snap_two_anchor_solver.hpp | 91 lines | Endpoint provider/request/result contract for two-ended rigid-piece placement | Changing portable two-anchor API |
| Modules/PlacementSnap/src/world_snap_two_anchor_solver.cpp | 251 lines | Deterministic ordered-pair search, length/pitch/axis/load/socket/revision gates, commit revalidation | Changing portable two-anchor behavior |
| Modules/StructuralIntegrity/include/world_mechanics/structural_integrity.hpp | 69 lines | Graph node/edge/assessment plus typed graph snapshot API | Changing portable support graph surface |
| Modules/StructuralIntegrity/src/structural_integrity.cpp | 271 lines | Graph mutation/revision, deterministic aggregate-load assessment, staged candidate-graph restore | Changing portable support assessment/restore |
| Modules/CMakeLists.txt | 51 lines | Portable source target and test-runner registrations | Adding/removing portable source files |
| Modules/tests/module_tests.cpp | 1275 lines | Portable assertion source for scan/terrain/snap/excavation/support/environment/crafting/restore/fire/traps/two-anchor/ring | Updating portable regression source |

### B. Unreal WorldSnap plugin and log placement

| File | Current size | Responsibility / changed behavior | Read only when |
|---|---:|---|---|
| Plugins/WorldSnap/Source/WorldSnap/Public/WorldSnapTypes.h | 81 lines | Generic point/query/result schema; availability, occupancy and load-bearing flags | Changing generic candidate contract |
| Plugins/WorldSnap/Source/WorldSnap/Private/WorldSnapResolver.cpp | 138 lines | Candidate gate and deterministic scoring for normal/axis/distance | Changing generic resolver ranking |
| Plugins/WorldSnap/Source/WorldSnap/Public/WorldSnapResolver.h | 20 lines | Static generic resolver API | Changing public resolver signature |
| Plugins/WorldSnap/Source/WorldSnap/Public/WorldSnapTwoAnchorSolver.h | 95 lines | UE anchor/query/result; occupied, target-compatible, support-valid/capacity and gap/penetration tuning | Changing UE two-anchor contract |
| Plugins/WorldSnap/Source/WorldSnap/Private/WorldSnapTwoAnchorSolver.cpp | 165 lines | UE-unit validation, endpoint-derived pitch/full rotation and endpoint alignment | Changing UE two-anchor math |
| Plugins/WorldSnap/Source/WorldSnap/Private/Tests/WorldSnapResolverTests.cpp | 199 lines | Editor Automation source for generic resolver | Changing generic UE automation |
| Plugins/WorldSnap/Source/WorldSnap/Private/Tests/WorldSnapTwoAnchorSolverTests.cpp | 197 lines | Editor Automation source for level/slope/length/readiness/capacity/gap/penetration | Changing two-anchor UE automation |
| Source/MyProject2/ForestCharacter.h | 623 lines | Public player build/carry/test contracts and slope/jacking control API | Changing Character API, not for general implementation details |
| Source/MyProject2/ForestCharacter.cpp | 5284 lines | Shared snap provider use, placement preview and targeted ground-to-post two-anchor slope placement | For solver caller, read only lines 1518–1545, 1683 onward, 1747–1814 or jacking functions 1234–1305 |
| Source/MyProject2/Phase2_Transport/CarvableLogJackingComponent.h | 54 lines | Lift one endpoint and real support-piece API | Changing jacking API |
| Source/MyProject2/Phase2_Transport/CarvableLogJackingComponent.cpp | 177 lines | Endpoint geometry, module-fit checks, support graph validation and rollback | Changing jacking implementation |

### C. Actual UE player-placed stone ring and support

| File | Current size | Responsibility / changed behavior | Read only when |
|---|---:|---|---|
| Source/MyProject2/StonePhaseB_Placement/StonePlacementComponent.cpp | 729 lines | Face/socket preview, full-volume overlap, matching socket search, structural connection, final install/rollback, graph invalidation, installed-stone ring recognition | Read only 394–589 for connection or 591–656 for commit/undo |
| Source/MyProject2/StonePhaseA_Mining/StoneActor.cpp | 115 lines | Stone actor structural-state transition, shape/damage and support invalidation | Changing stone damage/install/fall lifecycle |
| Source/MyProject2/StonePhaseF_Environment/StoneShapeRecognitionSubsystem.h | 61 lines | UE world-subsystem pattern recognition contract and tuning | Changing runtime hearth recognition parameters |
| Source/MyProject2/StonePhaseF_Environment/StoneShapeRecognitionSubsystem.cpp | 223 lines | UE ring geometry and event-driven manual fireplace creation after stone installation | Changing actual gameplay ring recognition |
| Source/MyProject2/Campfire.h | 84 lines | UE campfire Actor contract (heat, fuel, visible/external ring modes) | Changing Campfire runtime API |
| Source/MyProject2/Campfire.cpp | 234 lines | Built-in vs player-placed ring visual behavior and heat/fire Tick | Changing live campfire actor behavior |
| Source/MyProject2/StructureLink.h | 86 lines | UE shared mixed-material graph API/feature info/support limits | Changing StructureLink public surface |
| Source/MyProject2/StructureLink.cpp | 573 lines | Installed wood+stone collection, geometry connections, beam span/overhang, feature analysis, support settlement, candidate support query | Changing actual UE structural authority |

### D. UE trap and defense code

| File | Current size | Responsibility / changed behavior | Read only when |
|---|---:|---|---|
| Source/MyProject2/Phase6_Utility/TrapActor.h | 66 lines | World trap/payload/impulse/damage/cooldown properties | Changing payload trap API |
| Source/MyProject2/Phase6_Utility/TrapActor.cpp | 109 lines | Pressure/proximity overlap, mechanism gate, physical payload release, spike-pit launch | Changing actual world trap behavior |
| Source/MyProject2/StickPhaseS6_Traps/TrapMechanismComponent.h | 72 lines | UE trigger mode and lifecycle-state public contract | Changing UE trap authority |
| Source/MyProject2/StickPhaseS6_Traps/TrapMechanismComponent.cpp | 102 lines | UE trigger source, cooldown timer, damage/broadcast and consumption | Changing UE trigger lifecycle |
| Source/MyProject2/StickPhaseS6_Traps/SpringTrap.cpp | 76 lines | Root component fix; only supported target + accepted mechanism trigger can launch | Changing spring trap physics behavior |
| Source/MyProject2/StickPhaseS2_Processing/StickSharpeningComponent.cpp | 68 lines | Multi-hit stick sharpening progress and shape transition | Changing sharpening rules |
| Source/MyProject2/StickPhaseS5_Defense/StakeActor.h | 71 lines | Stake damage/sharpen/reinforce state | Changing stake public state |
| Source/MyProject2/StickPhaseS5_Defense/StakeActor.cpp | 153 lines | Sharpened damage volume and physical stone reinforcement | Changing stake durability/damage |
| Source/MyProject2/StickPhaseS5_Defense/StakeReinforcementComponent.cpp | 36 lines | Physical unique stone validation/consumption and optional count-only path | Changing stake recipe/resource debit |
| Source/MyProject2/StickPhaseS5_Defense/StakeWallComponent.cpp | 77 lines | Transactional staged stake-wall build and rollback | Changing palisade build transaction |
| Source/MyProject2/StickPhaseS5_Defense/WattleFenceComponent.cpp | 202 lines | Staged fence posts/weaves, socket connections, rollback and clay infill | Changing wattle/clay runtime lifecycle |

### E. Wood conversion and project plan

| File | Current size | Responsibility / changed behavior | Read only when |
|---|---:|---|---|
| Source/MyProject2/RawLogShapes.cpp | 327 lines | Shape/action preflight, staged split/trim/hewn outputs and rollback before source conversion commits | Changing raw-log conversion |
| Docs/SOTF_MECHANICS_EXPANSION_PLAN.md | existing | Main behavior reference/acceptance design for SotF-inspired logs, building, support, openings, logistics and defenses | Checking design intent before implementation |
| Docs/SOTF_MECHANICS_IMPLEMENTATION_BACKLOG.md | existing | Workstreams A–H and expected gates/acceptance criteria | Choosing next workstream |
| Docs/WorldSnapLinkedPackage.md | existing | WorldSnap plugin responsibility and existing integration rationale | Changing shared snap architecture |
| Docs/KARARLAR.md | existing | Decision ledger | Checking previously locked choices |
| Modules/PlacementSnap/SPEC.md | 37 lines | Generic provider-to-commit pipeline and two-anchor portable-vs-UE parity note | Changing generic placement contract |
| Docs/CLAUDE_CODE_START_HERE.md | new | Lowest-token entry point and work protocol | Always read first |
| Docs/CLAUDE_CODE_SOTF_LINE_BY_LINE_HANDOFF.md | updated | Detailed code map, call flow, range-by-range explanations, owners and open risks | Read only the matching section |
| Docs/CLAUDE_CODE_SOTF_CHANGE_MANIFEST.md | this file | All-code path map and system ownership index | Use for file discovery |

## 2. Actual call paths

### Actual UE manual hearth creation
1. UStonePlacementComponent::BeginPlacement starts a placed stone preview.
2. UpdatePlacementTarget / ApplySelectedPlacementOption derives transform from target face/socket.
3. TryConnectStructuralTarget (begin line 394) validates target structural interface, source definition/face/socket, target socket occupancy and normal/distance fit.
4. ConfirmPlacement (line 591) checks candidate volume, commits transform and required graph connection, transitions stone to Installed; failure rolls connection/placement back.
5. On success it releases the carry reference, stores placement history, calls StructureLink::MarkChanged / Settle, and triggers UStoneShapeRecognitionSubsystem::TryRecognizeManualFireplace with that newly installed stone.
6. UE subsystem assesses the local ring and calls ACampfire::BuildWithExternalStoneRing only if geometry is accepted and no fireplace already occupies that area.
Do not claim Modules/GameplayIntegration/StoneFireplaceLifecycle is on this path until an adapter is authored.

### Actual UE two-anchor slope placement
1. ForestCharacter computes a ground endpoint + top of an upright anchored post while in slope mode (ForestCharacter.cpp 1747–1814).
2. FWorldSnapTwoAnchorSolver::Solve validates endpoint length, availability, occupied/compatible/support/load and angle constraints.
3. Successful result becomes the current slope preview transform; EvaluateSupportPreview checks actual StructureLink support.
4. The exact final action must still be checked for preview/commit parity when touching this path; portable ValidateCommit is not used by the UE plugin solver.
Do not claim all arbitrary beam/log anchor providers have been integrated; this is a targeted scenario.

### Actual UE trap trigger
1. ATrapActor overlap routes pressure/proximity into Trigger.
2. Trigger copies tuning into UTrapMechanismComponent and invokes its trigger gate.
3. Only accepted mechanism trigger broadcasts OnTrapTriggered.
4. HandleMechanismTriggered releases an explicitly assigned physical log payload or launches a spike-pit victim.
5. ASpringTrap similarly waits for Mechanism->Trigger to accept before applying impulse/LaunchCharacter.
Portable wm::TrapLifecycle is not on this call path.

### Actual UE support
1. StoneActor/log placement updates installation/connection state.
2. StructureLink collects installed logs and installed stones, builds geometry nodes/edges, evaluates support and invalidates world-scoped feature cache.
3. Settle repeats a bounded failure loop; unsupported logs fall/snap and unsupported stone actors switch to Falling.
Portable StructuralGraph snapshot/mutation code is not the live UE support registry.

## 3. Recent commit groups

The branch has a longer mechanics history than the latest commit burst. Use commit history for exact diffs; the following anchors identify the implementation clusters without requiring Claude Code to read every commit:
- Local portable stone-ring recognition: 655998aa4d5cab0bf5342b3b379ccc70f6d90de8; 3e07f468f399ed63837707eb7ac8a9c155b38cc8; 623debc9ad9e4247b566f7299e556e4a98dc7e07; 05fb4a23b50a8fde8ce1ce3dc3b280f6889629dc; 6cae18ae32cfb2b486977a79c9fdfdde436d10a8.
- Portable fireplace/trap lifecycle, CMake and test source: a6db887ae8d49b1be4e63a972890ee4132055418; dd44505e3d35c119d1f804d65ef148a7fc21cc2c; 0a55ae88dfc4eefec28b146b50e3c87e5521fffe; c4b38ab57e953563f79e382e96bf69f6b20e1d6e; ded4bff9cdd9647403723111900b878f686600cb; 8eb12390fda0565391b95f9bb207a6a68c40c120; b4e616f8fb47aeb80495930b4539fa824e1969ff; c15a757f4730bf35da808efbb36e717c9a629618; fa463137c3475d17b2725c027cdb38bff05ec4ab.
- Portable structural mutation/snapshot: 0e77346c4ee1c5e1a4c7e18c337c8f8159ad2e3d; c7ba8081976052df4ab31825fd291a05420935be; b84b0903152946e95821798b7ea4ee49f15116fd; e9b04928f2ea075a44f0e4427b7cb94465a18d33; 9627be9c429db68a3a5035f80bfe952e8c47db5b.
- Portable two-anchor solver: d313ce7a132bb22db03cbc5a12a056bd22be74af; 7c73016cdc8ecb69aaa00ce304c005854b6f955f; 7f602b8f7fb6029cd7d7f57c2a257c6688cdad3d; 81a7a2c0c5cbf6d655eae2648be6883e5bdae257; e912a50e6ad843ef3bb2c34b1f7669f43cb4205c.
- UE two-anchor gates/automation and ForestCharacter bridge: 1f7ca07332992cde839b8fd4b106778ad193eddd; 2b9444ef56cd4e28e418e20ce8b35608e262c6a4; d39de8e9735defdd328c5286d4ef45ff28e77aef; 24a79bf3f1e1ec6389397b06de3fcfdbdd6b163b; 72e26bdaf41f31b440d5d1b214ac96cfe66eda18.
- Existing UE branch features include generic WorldSnap routing, player-placed hearth recognition, mixed wood/stone support, log jacking, transactional raw-log conversion, payload traps, stake reinforcement/walls and wattle/clay fences. The file table above is the source locator; use the exact subsystem block rather than re-reading repository-wide history.

## 4. Verification status — do not overstate

- Tests: not executed in this work segment. There are portable test source functions and Unreal Automation declarations, but no new PASS results.
- Build: no CMake build, CTest, UBT/UHT, editor automation run or PIE session was performed in this work segment.
- Unreal integration: generic snap is in the plugin; the specific slope solver is called by ForestCharacter; manual stone ring recognition runs through UE StonePlacementComponent and UStoneShapeRecognitionSubsystem; mixed wood/stone support runs through StructureLink. Portable fireplace/trap/structural graph/two-anchor reference implementations remain separately compiled source until an explicit adapter is implemented.
- Performance: no CPU profiling or world-size benchmark was run. Do not claim a measured performance improvement.
- Save: typed SaveRecord/Snapshot state contracts exist in portable models, but no end-to-end UE SaveGame serializer, binary codec, crash recovery or actor/inventory transaction validation has been demonstrated.
