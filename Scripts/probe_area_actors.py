"""Actors whose bounds touch a box: py ".../probe_area_actors.py" xmin,xmax,ymin,ymax (cm) -> Saved/area_actors.json"""
import json, os, sys
import unreal
x0, x1, y0, y1 = [float(v) for v in sys.argv[1].split(',')]
out = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    o, e = a.get_actor_bounds(False)
    if o.x + e.x >= x0 and o.x - e.x <= x1 and o.y + e.y >= y0 and o.y - e.y <= y1 and e.x < 50000:
        m = a.static_mesh_component.static_mesh if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh else None
        out.append({'label': a.get_actor_label(), 'mesh': m.get_path_name() if m else None, 'nanite': m.get_editor_property('nanite_settings').enabled if m else None,
                    'c': [round(o.x), round(o.y), round(o.z)], 'e': [round(e.x), round(e.y), round(e.z)]})
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'area_actors.json'), 'w'), indent=1)
