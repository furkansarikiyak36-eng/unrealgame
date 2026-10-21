"""Shrubs inside the grass test area of /Game/Maps/PerfTest.

Run in the open editor's Output Log (Cmd):
    py "C:/Users/merts/Downloads/MyProject2/Scripts/build_area_shrubs.py" <shrubs> [half_side_m] [folders, comma separated]
Default: 40 shrubs, 23 m (the 50 x 50 m PCG grass volume), European Spindle + Elderberry. Only variants at least
80 cm tall, scaled 0.9-1.3 so leaves keep their real size. Clumps of 2-4, up to 3 m apart, kept inside the square.
One actor, one HISM per mesh, shadows on, out of the distance fields, no collision.
Replaces the previous layer (tag AreaShrubs) and saves the map. Report: Saved/area_shrubs.json.
"""
import collections
import json
import math
import os
import random
import sys

import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))

TAG = 'AreaShrubs'
MS = '/Game/Fab/Megascans/Plants/'
MIN_HEIGHT = 80.0

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    half = (float(sys.argv[2]) if len(sys.argv) > 2 else 23.0) * 100.0
    folders = sys.argv[3].split(',') if len(sys.argv) > 3 else ['European_Spindle_wk1ncbxja', 'Elderberry_wfzobb2ia']
    for a in actors.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            actors.destroy_actor(a)
    meshes = []
    for f in folders:
        for d in registry.get_assets_by_path(MS + f, recursive=True):
            if str(d.asset_class_path.asset_name) == 'StaticMesh':
                m = unreal.load_asset(str(d.package_name))
                b = m.get_bounding_box()
                if b.max.z - b.min.z >= MIN_HEIGHT:
                    meshes.append(m)
    rng = random.Random(count)
    xforms = [[] for _ in meshes]
    placed = 0
    while placed < count:
        cx, cy = rng.uniform(-half, half), rng.uniform(-half, half)
        for _ in range(min(rng.randint(2, 4), count - placed)):
            d, a = rng.uniform(0, 300.0), rng.uniform(0, 2 * math.pi)
            x = max(-half, min(half, cx + d * math.cos(a)))
            y = max(-half, min(half, cy + d * math.sin(a)))
            s = rng.uniform(0.9, 1.3)
            xforms[rng.randrange(len(meshes))].append(unreal.Transform(
                unreal.Vector(x, y, 0.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=rng.uniform(0, 360)), unreal.Vector(s, s, s)))
            placed += 1
    actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    actor.set_actor_label('AreaShrubs')
    actor.tags = [TAG]
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    root = sub.k2_gather_subobject_data_for_instance(actor)[0]
    per_mesh = collections.Counter()
    for m, xs in zip(meshes, xforms):
        if not xs:
            continue
        handle, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent, blueprint_context=None))
        c = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
        c.set_static_mesh(m)
        c.set_cast_shadow(True)
        c.set_editor_property('affect_distance_field_lighting', False)
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        c.add_instances(xs, False, True)
        b = m.get_bounding_box()
        per_mesh['{} ({} cm)'.format(m.get_name(), round(b.max.z - b.min.z))] = len(xs)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
    report = {'shrubs': placed, 'side_m': 2 * half / 100.0, 'folders': folders, 'meshes_used': dict(per_mesh)}
    with open(os.path.join(unreal.Paths.project_saved_dir(), 'area_shrubs.json'), 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[AreaShrubs] {} shrubs'.format(placed))


main()
