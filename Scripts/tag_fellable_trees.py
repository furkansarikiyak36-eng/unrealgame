"""Tags the Megaplant tree actors of the open level that are at least MIN_M tall with "Fellable" (the rest lose it)
and saves the map. py ".../tag_fellable_trees.py" [min_m]   Report: Saved/fellable_trees.json"""
import json, os, sys
import unreal
min_cm = (float(sys.argv[1]) if len(sys.argv) > 1 else 5.0) * 100.0
out = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if not isinstance(a, unreal.SkeletalMeshActor):
        continue
    m = a.skeletal_mesh_component.get_editor_property('skeletal_mesh_asset')
    if not m or not m.get_name().startswith('Tree_'):
        continue
    b = m.get_bounds()
    h = (b.origin.z + b.box_extent.z) * a.get_actor_scale3d().z
    tags = [t for t in a.tags if str(t) != 'Fellable']
    if h >= min_cm:
        tags.append('Fellable')
    a.tags = tags
    loc = a.get_actor_location()
    out.append({'label': a.get_actor_label(), 'mesh': m.get_name(), 'height_m': round(h / 100, 1),
                'fellable': h >= min_cm, 'x_m': round(loc.x / 100, 1), 'y_m': round(loc.y / 100, 1)})
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(w, w.get_path_name().split('.')[0])
json.dump(sorted(out, key=lambda t: -t['height_m']), open(os.path.join(unreal.Paths.project_saved_dir(), 'fellable_trees.json'), 'w'), indent=1)
