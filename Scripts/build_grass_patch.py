"""Satisfactory-style grass carpet test in /Game/Maps/PerfTest.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/build_grass_patch.py" [per_m2] [half_m]
Short Megascans grass clumps, per_m2 per square metre (default 4) over a (2 * half_m) square around the origin
(default 150 m, so 300 x 300 m). The cheap-grass recipe: drawn only within 40-60 m of the camera (fade by cull),
no shadow casting, out of the distance fields; the sun gets screen-space contact shadows so the grass still sits
on the ground. In the game this would be Landscape Grass, which spawns the same instances only around the player,
so memory does not grow with the map size; this patch measures the look and the render cost.
Also removes the Megaplant tree test layer (tag ShrublandTrees). Report: Saved/grass_patch.json.
"""
import json
import os
import random
import sys
import time

import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))

TAG = 'GrassPatch'
MS = '/Game/Fab/Megascans/Plants/'
GRASSES = ['Kikuyu_Grass_wiljearqx', 'Wild_Grass_vczndjqja']
MAX_HEIGHT = 70.0                    # cm: only the short clumps, a mown-meadow carpet
CULL_START, CULL_END = 4000.0, 6000.0
BATCH = 100000

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()


def main():
    per_m2 = float(sys.argv[1]) if len(sys.argv) > 1 else 4.0
    half = (float(sys.argv[2]) if len(sys.argv) > 2 else 150.0) * 100.0
    removed = 0
    for a in actors.get_all_level_actors():
        tags = [str(t) for t in a.tags]
        if TAG in tags or 'ShrublandTrees' in tags:
            removed += 1
            actors.destroy_actor(a)
    meshes, heights = [], {}
    for g in GRASSES:
        for d in registry.get_assets_by_path(MS + g, recursive=True):
            if str(d.asset_class_path.asset_name) != 'StaticMesh':
                continue
            m = unreal.load_asset(str(d.package_name))
            b = m.get_bounding_box()
            heights[m.get_name()] = round(b.max.z - b.min.z)
            if b.max.z - b.min.z <= MAX_HEIGHT:
                meshes.append(m)
    actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    actor.set_actor_label('GrassPatch')
    actor.tags = [TAG]
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    root = sub.k2_gather_subobject_data_for_instance(actor)[0]
    comps = []
    for m in meshes:
        handle, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent, blueprint_context=None))
        c = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
        c.set_static_mesh(m)
        c.set_cull_distances(CULL_START, CULL_END)
        c.set_cast_shadow(False)
        c.set_editor_property('affect_distance_field_lighting', False)
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        comps.append(c)
    total = int(per_m2 * (2 * half / 100.0) ** 2)
    rng = random.Random(7)
    t0 = time.time()
    per = total // len(comps)
    for c in comps:
        left = per
        while left > 0:
            k = min(BATCH, left)
            xs = []
            for _ in range(k):
                s = rng.uniform(0.8, 1.2)
                xs.append(unreal.Transform(unreal.Vector(rng.uniform(-half, half), rng.uniform(-half, half), 0.0),
                                           unreal.Rotator(roll=0.0, pitch=0.0, yaw=rng.uniform(0, 360)), unreal.Vector(s, s, s)))
            c.add_instances(xs, False, True)
            left -= k
    # Contact shadows on the sun: grass that casts no shadow map still gets a dark base.
    suns = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.DirectionalLight)]
    for s in suns:
        s.light_component.set_editor_property('contact_shadow_length', 0.05)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
    report = {'instances': per * len(comps), 'per_m2': per_m2, 'side_m': 2 * half / 100.0, 'meshes': len(comps),
              'mesh_heights_cm': heights, 'cull_cm': [CULL_START, CULL_END], 'removed_actors': removed,
              'suns_contact_shadow': len(suns), 'seconds': round(time.time() - t0, 1)}
    with open(os.path.join(unreal.Paths.project_saved_dir(), 'grass_patch.json'), 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[GrassPatch] {} instances, {} meshes'.format(report['instances'], len(comps)))


main()
