"""Read only: every actor of the open level with what is needed to judge it. Writes Saved/scene_analysis.json.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/analyze_scene.py"
Per actor: label, class, folder, tags, location/rotation/scale, world bounds, and for mesh actors the mesh, its Nanite
state and triangle count, collision profile, mobility, cast shadow and material overrides.
"""
import json
import os

import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), 'scene_analysis.json')


def vec(v):
    return [round(v.x, 1), round(v.y, 1), round(v.z, 1)]


def mesh_info(mesh, cache):
    path = mesh.get_path_name()
    if path in cache:
        return cache[path]
    info = {'path': path}
    if isinstance(mesh, unreal.StaticMesh):
        try:
            info['nanite'] = bool(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_nanite_settings(mesh).enabled)
            info['nanite_tris'] = mesh.get_num_nanite_triangles()
            info['tris'] = mesh.get_num_triangles(0)
            info['lods'] = mesh.get_num_lods()
        except Exception as error:
            info['err'] = str(error)[:100]
    cache[path] = info
    return info


def main():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    cache = {}
    rows = []
    for actor in actors:
        origin, extent = actor.get_actor_bounds(False)
        row = {
            'label': actor.get_actor_label(),
            'class': actor.get_class().get_name(),
            'folder': str(actor.get_folder_path()),
            'tags': [str(t) for t in actor.tags],
            'loc': vec(actor.get_actor_location()),
            'rot': [round(actor.get_actor_rotation().pitch, 1), round(actor.get_actor_rotation().yaw, 1), round(actor.get_actor_rotation().roll, 1)],
            'scale': vec(actor.get_actor_scale3d()),
            'min': vec(origin - extent),
            'max': vec(origin + extent),
            'hidden': actor.is_hidden_ed(),
        }
        smc = actor.get_component_by_class(unreal.StaticMeshComponent)
        skc = actor.get_component_by_class(unreal.SkeletalMeshComponent)
        comp = smc or skc
        if smc and smc.static_mesh:
            row['mesh'] = mesh_info(smc.static_mesh, cache)
        elif skc and skc.get_skeletal_mesh_asset():
            row['mesh'] = mesh_info(skc.get_skeletal_mesh_asset(), cache)
        elif comp:
            row['mesh'] = None
        if comp:
            row['collision'] = str(comp.get_collision_profile_name())
            row['mobility'] = str(comp.mobility).split('.')[-1]
            row['shadow'] = comp.cast_shadow
            row['overrides'] = len(comp.override_materials) if hasattr(comp, 'override_materials') else 0
        rows.append(row)
    report = {'map': world.get_path_name(), 'actors': rows}
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[AnalyzeScene] {} actors -> {}'.format(len(rows), OUT))


main()
