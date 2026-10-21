"""Read-only, PIE: what a downward line trace hits at x,y (all hits, WorldStatic objects).
py ".../pie_trace_probe.py" x,y -> Saved/pie_trace_probe.json"""
import json, os, sys
import unreal
x, y = [float(v) for v in sys.argv[1].split(',')]
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
hits = unreal.SystemLibrary.line_trace_multi_for_objects(world, unreal.Vector(x, y, 3000), unreal.Vector(x, y, -1500),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], False, [], unreal.DrawDebugTrace.NONE, True)
out = []
for h in hits or []:
    t = h.to_tuple()
    out.append([str(v) for v in t[:3]] + [str(t[4])[:120], t[9].get_name() if t[9] else None, t[10].get_name() if t[10] else None])
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_trace_probe.json'), 'w'), indent=1)
