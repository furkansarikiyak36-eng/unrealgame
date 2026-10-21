# Log Placement & Snap System — Technical Plan

**Target:** Unreal Engine 5.8 / C++  
**Branch:** feature/log-snap-system-v01  
**Status:** Code draft committed to the feature branch; Unreal Editor/UBT build and gameplay validation still required.

## Architecture

1. Raycast target acquisition selects a target actor and surface normal. It does not directly define the final transform.
2. Predefined snap points: LogSnapSystem builds deterministic module-line points on the top, bottom, front and back faces of a horizontal log, plus two end points.
3. Resolver: chooses a compatible point using axis distance and hit-normal alignment. Equal scores preserve generation order to prevent random flips at ties. Short logs without an interior module line receive a centre snap.
4. Placement rules: existing project functions continue to calculate stack, end-to-end, side/T, corner, and diagonal transforms.
5. Connection/support: StructureLink remains the authority for structural support and settling. Its graph is currently reconstructed from installed log geometry and contact; this branch does not yet add serialized socket IDs to save data.
6. Preview/commit: a shared placement-result snapshot carries mode, transform, target, snap ID, validity and support state. Preview and commit use the same transform resolver; support is force-revalidated on commit.

## First-pass rules

- Stack: a log on a horizontal log uses a predefined top-face module point and becomes anchored when committed.
- Vertical post: the base uses a controlled embed/seat depth rather than perfectly tangent contact.
- End-to-end: use EndA/EndB and align axes.
- Side/T connection: use a side-face point and a 90-degree connection rule.
- Diagonal: use a shared reference axis and the nearest valid +45/-45 sector. The sign is measured in that frame, not from absolute world yaw.
- Invalid placement: no compatible snap means no snap placement; preview should show why it is invalid.
- Support loss: structure changes (removal, breakage or burning) trigger support reevaluation. Direct contact is not a serialized connection.

## Performance

- Do not iterate all ACarvableLog actors from the placement-preview path when a local spatial query is sufficient.
- Cache support preview by piece, target, mode, snap ID, transform and structural graph version. Reuse unchanged results; throttle changed-candidate graph checks to 10 Hz; always perform a fresh support check on commit.
- The three all-log iterators inside ForestCharacter placement preview were replaced by local overlap queries. Other systems, including the structural graph itself, still perform global scans when rebuilding the graph.
- Keep StructureLink::Settle as a structural-change operation, not a per-frame operation.
- Profile with Unreal Insights before making performance claims.

## Automation tests

Automation tests cover axis-yaw folding, standard angle sectors, nearest compatible snap selection, and deterministic tie-breaking. World tests are still needed for top stacking and anchoring, end-to-end connection, complementary +45/-45 courses, post embed depth, support removal/fall propagation, save/load reconstruction, and preview-cost profiling.

## Known limitations

- Existing specialized placement paths (beam, brace, floor, corner and carving seats) are retained rather than rewritten wholesale.
- The structural graph is geometry-derived; persistent explicit connection IDs are a future schema change.
- No Unreal Editor/UBT build has been run by this code-writing session.
