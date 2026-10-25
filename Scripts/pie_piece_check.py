"""PIE check: material slots of raw-log building pieces, and the yaw of each lying piece relative to its nearest lying
neighbour (snapping should give multiples of 90). py ".../pie_piece_check.py" -> Saved/pie_piece_check.json"""
import json, math, os
import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {'pieces': []}
if world:
    logs = [l for l in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CarvableLog) if l.get_attach_parent_actor() is None]
    for l in logs:
        comp = l.get_component_by_class(unreal.PrimitiveComponent)
        mats = [comp.get_material(i).get_name() if comp.get_material(i) else None for i in range(comp.get_num_materials())]
        sm = l.get_editor_property('source_mesh')
        others = [o for o in logs if o != l]
        rel = None
        if others:
            n = min(others, key=lambda o: (o.get_actor_location() - l.get_actor_location()).length())
            d = (l.get_actor_rotation().yaw - n.get_actor_rotation().yaw) % 90.0
            rel = round(min(d, 90.0 - d), 1)
        out['pieces'].append([sm.get_name() if sm else None, mats, round(l.get_actor_rotation().yaw, 1), rel,
                              [round(v) for v in (l.get_actor_location().x, l.get_actor_location().y, l.get_actor_location().z)]])
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_piece_check.json'), 'w'), indent=1)
