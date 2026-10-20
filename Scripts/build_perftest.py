"""Builds /Game/Maps/PerfTest layer by layer for a performance test on a 3.5 x 3.5 km (12.25 km2) flat plane.

Run in the open editor's Output Log (Cmd), one layer per run:
    py "C:/Users/merts/Downloads/MyProject2/Scripts/build_perftest.py" base      new map, ground, lights
    py "...build_perftest.py" grass     Megascans grasses, instanced (HISM), culled at GRASS_CULL
    py "...build_perftest.py" plants    Megascans plants and shrubs, instanced, culled at PLANT_CULL
    py "...build_perftest.py" trunks    Megaplant trees (< 5 M variants) as actors, branches and leaves hidden
                                        (r.Nanite.Culling.ShowAssemblyParts 0)
    py "...build_perftest.py" stones    Fab rocks and stones (Nanite), instanced
    py "...build_perftest.py" leaves    r.Nanite.Culling.ShowAssemblyParts 1
Each run replaces its own layer (actor tagged PerfLayer_<name>) and saves the map. Counts and timings go to
Saved/perftest_<layer>.json. Instances are seeded (random.Random(layer)) so a re-run gives the same scene.
"""
import json
import math
import os
import random
import sys
import time

import unreal

MAP = '/Game/Maps/PerfTest'
DEST = '/Game/PerfTest'
HALF = 175000.0                  # 3.5 km side
GROUND_Z = 0.0
GRASS_COUNT, GRASS_CULL = 1000000, 15000.0
PLANT_COUNT, PLANT_CULL = 200000, 30000.0
TREE_COUNT = 1000
STONE_COUNT = 50000
BATCH = 50000
MS = '/Game/Fab/Megascans/'
GRASSES = ['Wild_Grass_vczndjqja', 'Kikuyu_Grass_wiljearqx', 'Dry_Grass_tbbqejqr', 'Thatching_Grass_uddmcgbia',
           'Wheat_Grass_tdcrdbur', 'Ribbon_Grass_tbdpec3r', 'Ground_Cover_sfcnq', 'Curbside_Weeds_ujzjfexja']
STONES_3D = ['Small_Stones_Pack_vcljbb1iw', 'Small_Stones_Pack_vdjkbfmiw', 'Mossy_Stones_Pack_tliiadmva',
             'Nordic_Beach_Rocks_ulznddxva', 'Military_Trenches_Scatter_Rock_S_10_ydxsabu', 'Mossy_Forest_Boulder_wfstcdnaw',
             'Sandstone_Boulder_vmjjfdm', 'Beach_Boulder_ukopeess', 'Rock_shopk']
MEGA_TREES = {'Tree_Norway_Maple_Sapling_01_': 'ABCD', 'Tree_Japanese_Cypress_01_': 'CEFG',
              'Tree_Rowan_Sapling_01_': 'ABCD', 'Tree_Baltic_Pine_Sapling_01_': 'ABCD', 'Tree_Dead_Pine_': 'ABCDE',
              'Tree_European_Aspen_01_': 'D'}

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()


def out(layer, data):
    path = os.path.join(unreal.Paths.project_saved_dir(), 'perftest_{}.json'.format(layer))
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
    unreal.log('[PerfTest] {} {}'.format(layer, json.dumps({k: v for k, v in data.items() if k != 'meshes'})))


def meshes_under(path):
    return [unreal.load_asset(str(d.package_name)) for d in registry.get_assets_by_path(path, recursive=True)
            if str(d.asset_class_path.asset_name) == 'StaticMesh']


def clear(tag):
    for a in actors.get_all_level_actors():
        if tag in [str(t) for t in a.tags]:
            actors.destroy_actor(a)


def save():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)


def instanced(layer, meshes, count, cull, rng, scale=(0.8, 1.25), random_tilt=0.0):
    """One actor, one HISM per mesh, `count` instances spread uniformly over the plane."""
    tag = 'PerfLayer_' + layer
    clear(tag)
    actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    actor.set_actor_label('PerfLayer_' + layer)
    actor.tags = [tag]
    # Components have to be registered on the actor: use the SubobjectDataSubsystem.
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sub.k2_gather_subobject_data_for_instance(actor)
    root_handle = handles[0]
    real = []
    for i, mesh in enumerate(meshes):
        params = unreal.AddNewSubobjectParams(parent_handle=root_handle,
                                              new_class=unreal.HierarchicalInstancedStaticMeshComponent,
                                              blueprint_context=None)
        handle, fail = sub.add_new_subobject(params)
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        comp.set_static_mesh(mesh)
        if cull:
            comp.set_cull_distances(0, cull)
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        comp.set_editor_property('cast_shadow', True)
        real.append(comp)
    per = [count // len(real) + (1 if i < count % len(real) else 0) for i in range(len(real))]
    t0 = time.time()
    total = 0
    for comp, n in zip(real, per):
        done = 0
        while done < n:
            k = min(BATCH, n - done)
            xforms = []
            for _ in range(k):
                s = rng.uniform(*scale)
                rot = unreal.Rotator(roll=rng.uniform(-random_tilt, random_tilt), pitch=rng.uniform(-random_tilt, random_tilt), yaw=rng.uniform(0, 360))
                xforms.append(unreal.Transform(unreal.Vector(rng.uniform(-HALF, HALF), rng.uniform(-HALF, HALF), GROUND_Z),
                                               rot, unreal.Vector(s, s, s)))
            comp.add_instances(xforms, False, True)
            done += k
        total += n
    return {'layer': layer, 'instances': total, 'meshes': [m.get_name() for m in meshes], 'per_mesh': per,
            'cull_cm': cull, 'seconds': round(time.time() - t0, 1)}


def base():
    unreal.EditorLoadingAndSavingUtils.new_map_from_template('/Engine/Maps/Templates/Template_Default', False)
    for a in actors.get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor):
            actors.destroy_actor(a)          # the template's small floor
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mi_path = DEST + '/MI_PerfGround'
    if not unreal.EditorAssetLibrary.does_asset_exist(mi_path):
        mi = tools.create_asset('MI_PerfGround', DEST, unreal.MaterialInstanceConstant,
                                unreal.MaterialInstanceConstantFactoryNew())
        unreal.MaterialEditingLibrary.set_material_instance_parent(
            mi, unreal.load_asset(MS + 'Surfaces/Rocky_Forest_Floor_wjvpaff/Low/wjvpaff_tier_3/Materials/MI_wjvpaff'))
        # The cube's top face is one UV tile: 3.5 km / 2 m per repeat.
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, 'Tiling', 1750.0)
        unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    ground = actors.spawn_actor_from_object(cube, unreal.Vector(0, 0, GROUND_Z - 50.0))
    ground.set_actor_scale3d(unreal.Vector(HALF * 2 / 100.0, HALF * 2 / 100.0, 1.0))
    ground.static_mesh_component.set_material(0, unreal.load_asset(mi_path))
    ground.set_actor_label('PerfGround')
    ground.tags = ['PerfLayer_base']
    save()
    out('base', {'layer': 'base', 'map': MAP, 'area_km2': (HALF * 2 / 100000.0) ** 2})


def trunks():
    tag = 'PerfLayer_trunks'
    clear(tag)
    rng = random.Random('trunks')
    trees = []
    for d in registry.get_assets_by_path('/Game/Megaplant_Library', recursive=True):
        if str(d.asset_class_path.asset_name) != 'SkeletalMesh':
            continue
        name = str(d.asset_name)
        if any(name.startswith(p) and name[len(p):] in v for p, v in MEGA_TREES.items()):
            trees.append(unreal.load_asset(str(d.package_name)))
    unreal.SystemLibrary.execute_console_command(None, 'r.Nanite.Culling.ShowAssemblyParts 0')
    t0 = time.time()
    for i in range(TREE_COUNT):
        mesh = trees[i % len(trees)]
        a = actors.spawn_actor_from_object(mesh, unreal.Vector(rng.uniform(-HALF, HALF), rng.uniform(-HALF, HALF), GROUND_Z),
                                           unreal.Rotator(roll=0.0, pitch=0.0, yaw=rng.uniform(0, 360)))
        a.tags = [tag]
        a.set_folder_path('PerfTest/Trees')
        comp = a.skeletal_mesh_component
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    save()
    out('trunks', {'layer': 'trunks', 'actors': TREE_COUNT, 'variants': [t.get_name() for t in trees],
                   'seconds': round(time.time() - t0, 1), 'assembly_parts': 0})


def main():
    layer = sys.argv[1] if len(sys.argv) > 1 else 'base'
    if layer == 'base':
        base()
        return
    if layer == 'leaves':
        unreal.SystemLibrary.execute_console_command(None, 'r.Nanite.Culling.ShowAssemblyParts 1')
        out('leaves', {'layer': 'leaves', 'assembly_parts': 1})
        return
    if layer == 'trunks':
        trunks()
        return
    rng = random.Random(layer)
    if layer == 'grass':
        meshes = [m for g in GRASSES for m in meshes_under(MS + 'Plants/' + g)]
        data = instanced('grass', meshes, GRASS_COUNT, GRASS_CULL, rng)
    elif layer == 'plants':
        meshes = [m for sub in registry.get_sub_paths(MS + 'Plants', False)
                  if str(sub).rsplit('/', 1)[-1] not in GRASSES for m in meshes_under(str(sub))]
        data = instanced('plants', meshes, PLANT_COUNT, PLANT_CULL, rng)
    elif layer == 'stones':
        meshes = [m for s in STONES_3D for m in meshes_under(MS + '3D/' + s)]
        data = instanced('stones', meshes, STONE_COUNT, 0.0, rng, scale=(0.7, 1.6), random_tilt=20.0)
    else:
        raise RuntimeError('unknown layer ' + layer)
    save()
    out(layer, data)


main()
