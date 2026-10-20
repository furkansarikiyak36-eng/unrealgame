"""Shrubland in /Game/Maps/PerfTest: no grass, Megascans shrubs scaled up to small-tree height, in groves.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/build_shrubland.py" [groves]
Fewer instances, placed where they mean something: GROVES centres over the 1 x 1 km core; each grove has the tall
variants (native height >= 150 cm, scaled 1.2-1.6x, so 2-4.5 m) in the middle, shrubs of 80-150 cm at native size at
the rim, ferns, weeds and the small shrub variants under them and stones at the edge. Scale stays near 1 so leaves
keep their real size (scaling a 60 cm shrub to 5 m gave hand-sized leaves a metre wide).
Between groves the ground stays open. Boulders are scattered on their own.
Lighting cost per layer: shrubs and boulders cast shadows; ferns cast shadows but are culled at 120 m; plants stay
out of the distance fields (Lumen sees ground, rocks and the shrub canopy through the screen traces only).
Replaces the previous Shrubland layer (tag Shrubland) and saves the map. Report: Saved/shrubland.json.
"""
import json
import math
import os
import random
import sys
import time

import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))

CORE = 50000.0
MS = '/Game/Fab/Megascans/'
TAG = 'Shrubland'
SHRUBS = ['Raspberry_wf0oefeja', 'Elderberry_wfzobb2ia', 'European_Spindle_wk1ncbxja', 'Black_Locust_wdvhberja',
          'Dead_Shrubs_vdknafgha', 'Bigleaf_Hydrangea_vgztealha']
UNDER = ['Lady_Fern_wdvlditia', 'Beech_Fern_vmkpdbeia', 'Greater_Burdock_xh5pdgiqx', 'Common_Hemp_Nettle_xh1kdfbqx']
STONES = ['Small_Stones_Pack_vcljbb1iw', 'Small_Stones_Pack_vdjkbfmiw', 'Mossy_Stones_Pack_tliiadmva']
BOULDERS = ['Mossy_Forest_Boulder_wfstcdnaw', 'Sandstone_Boulder_vmjjfdm', 'Beach_Boulder_ukopeess', 'Rock_shopk']
# layer: cull cm (0 = none), cast shadow, in distance fields
SETTINGS = {'shrubs': (60000.0, True, False), 'under': (12000.0, True, False),
            'stones': (25000.0, True, True), 'boulders': (0.0, True, True)}

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()


def meshes(kind, folder):
    root = MS + ('Plants/' if kind == 'plants' else '3D/') + folder
    return [unreal.load_asset(str(d.package_name)) for d in registry.get_assets_by_path(root, recursive=True)
            if str(d.asset_class_path.asset_name) == 'StaticMesh']


def height(mesh):
    b = mesh.get_bounding_box()
    return max(1.0, b.max.z - b.min.z)


def layer_actor(name, mesh_list):
    actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    actor.set_actor_label('Shrubland_' + name)
    actor.tags = [TAG]
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    root = sub.k2_gather_subobject_data_for_instance(actor)[0]
    cull, shadow, df = SETTINGS[name]
    comps = []
    for m in mesh_list:
        handle, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent, blueprint_context=None))
        c = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
        c.set_static_mesh(m)
        if cull:
            c.set_cull_distances(0, cull)
        c.set_cast_shadow(shadow)
        c.set_editor_property('affect_distance_field_lighting', df)
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        comps.append(c)
    return comps


def main():
    groves = int(sys.argv[1]) if len(sys.argv) > 1 else 2500
    for a in actors.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            actors.destroy_actor(a)
    rng = random.Random(groves)
    t0 = time.time()
    shrub_meshes = [m for f in SHRUBS for m in meshes('plants', f)]
    sets = {'shrubs': [m for m in shrub_meshes if height(m) >= 80.0],
            'under': [m for f in UNDER for m in meshes('plants', f)] + [m for m in shrub_meshes if height(m) < 80.0],
            'stones': [m for f in STONES for m in meshes('3d', f)],
            'boulders': [m for f in BOULDERS for m in meshes('3d', f)]}
    xforms = {k: [[] for _ in v] for k, v in sets.items()}

    tall = [i for i, m in enumerate(sets['shrubs']) if height(m) >= 150.0]
    rim = [i for i, m in enumerate(sets['shrubs']) if height(m) < 150.0]

    def put(layer, x, y, scale=(0.8, 1.2), tilt=0.0, z=0.0, pick=None):
        i = rng.choice(pick) if pick else rng.randrange(len(sets[layer]))
        s = rng.uniform(*scale)
        xforms[layer][i].append(unreal.Transform(
            unreal.Vector(x, y, z), unreal.Rotator(roll=rng.uniform(-tilt, tilt), pitch=rng.uniform(-tilt, tilt), yaw=rng.uniform(0, 360)),
            unreal.Vector(s, s, s)))

    for _ in range(groves):
        cx, cy = rng.uniform(-CORE, CORE), rng.uniform(-CORE, CORE)
        r = rng.uniform(600.0, 2000.0)
        for _ in range(rng.randint(6, 26)):
            d = r * math.sqrt(rng.random())
            a = rng.uniform(0, 2 * math.pi)
            inner = d < r * 0.55
            if inner:
                put('shrubs', cx + d * math.cos(a), cy + d * math.sin(a), scale=(1.2, 1.6), pick=tall)
            else:
                put('shrubs', cx + d * math.cos(a), cy + d * math.sin(a), scale=(0.9, 1.3), pick=rim)
        for _ in range(rng.randint(25, 70)):
            d = r * 1.3 * math.sqrt(rng.random())
            a = rng.uniform(0, 2 * math.pi)
            put('under', cx + d * math.cos(a), cy + d * math.sin(a), scale=(0.8, 1.3))
        for _ in range(rng.randint(3, 10)):
            d = r * rng.uniform(0.9, 1.4)
            a = rng.uniform(0, 2 * math.pi)
            put('stones', cx + d * math.cos(a), cy + d * math.sin(a), scale=(0.7, 1.5), tilt=15.0, z=-3.0)
    for _ in range(groves // 4):
        put('boulders', rng.uniform(-CORE, CORE), rng.uniform(-CORE, CORE), scale=(0.8, 2.0), tilt=12.0,
            z=rng.uniform(-60.0, -15.0))

    report = {'groves': groves, 'layers': {}}
    for name, mesh_list in sets.items():
        comps = layer_actor(name, mesh_list)
        for c, xs in zip(comps, xforms[name]):
            if xs:
                c.add_instances(xs, False, True)
        report['layers'][name] = {'instances': sum(len(x) for x in xforms[name]), 'meshes': len(mesh_list),
                                  'cull_cm': SETTINGS[name][0], 'shadow': SETTINGS[name][1], 'distance_field': SETTINGS[name][2]}
    report['instances'] = sum(l['instances'] for l in report['layers'].values())
    report['seconds'] = round(time.time() - t0, 1)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
    with open(os.path.join(unreal.Paths.project_saved_dir(), 'shrubland.json'), 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[Shrubland] ' + json.dumps({k: v for k, v in report.items() if k != 'layers'}))


main()
