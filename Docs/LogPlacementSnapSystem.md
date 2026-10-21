# Log Placement & Snap System — Technical Plan

**Target:** Unreal Engine 5.8 / C++  
**Branch:** feature/log-snap-system-v01  
**Status:** Implementation draft; editor build and gameplay validation still required.

## Architecture

1. Raycast target acquisition selects a target actor and surface normal. It does not directly define the final transform.
2. Predefined snap points: LogSnapSystem builds deterministic module-line points on the top, bottom, front and back faces of a horizontal log, plus two end points.
3. Resolver: chooses a compatible point using axis distance and hit-normal alignment. Equal scores preserve generation order to prevent random flips at ties.
4. Placement rules: existing project functions continue to calculate stack, end-to-end, side/T, corner, and diagonal transforms.
5. Connection/support: StructureLink remains the authority for structural support and settling. Its graph is currently reconstructed from installed log geometry and contact; this branch does not yet add serialized socket IDs to save data.
6. Preview/commit: preview and commit should use the same chosen transform. Revalidate support on commit rather than trusting stale preview data.

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
- Cache support preview results briefly for small transform changes; always perform a fresh support check on commit.
- Keep StructureLink::Settle as a structural-change operation, not a per-frame operation.
- Profile with Unreal Insights before making performance claims.

## Automation tests

Initial tests cover axis-yaw folding and standard angle sectors. World tests are still needed for top stacking and anchoring, end-to-end connection, complementary +45/-45 courses, post embed depth, support removal/fall propagation, save/load reconstruction, and preview-cost profiling.

## Known limitations

- Existing specialized placement paths (beam, brace, floor, corner and carving seats) are retained rather than rewritten wholesale.
- The structural graph is geometry-derived; persistent explicit connection IDs are a future schema change.
- No Unreal Editor/UBT build has been run by this code-writing session.
