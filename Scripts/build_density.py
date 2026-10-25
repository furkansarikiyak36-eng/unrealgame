"""Instance stress test in /Game/Maps/PerfTest: N instances of Megascans vegetation in the 1 x 1 km core.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/build_density.py" <millions>
Replaces the previous DensityTest layer. Mix: 85 % grass, 10 % ground cover, 4 % mid plants, 1 % shrubs, uniform over
the core (the test is about how many instances the machine carries, not about looks). One actor per layer, one HISM
per mesh (few meshes per layer to keep the component count fixed between steps), no collision, cull distances as in
a game: grass 60 m, cover 80 m, mid 150 m, shrubs 300 m. Instances are added in batches so Python never holds more
than BATCH transforms. Writes Saved/density_<millions>.json with counts, the seconds it took and the process memory.
"""
import json
import os
import random
import sys
import time

import unreal

CORE = 50000.0
BATCH = 100000
MS = '/Game/Fab/Megascans/Plants/'
LAYERS = [  # name, share, cull cm, shadow, folders
    ('grass', 0.85, 6000.0, False, ['Wild_Grass_vczndjqja', 'Kikuyu_Grass_wiljearqx', 'Ribbon_Grass_tbdpec3r']),
    ('cover', 0.10, 8000.0, False, ['Wood_Sorrel_wcwmchzja', 'English_Ivy_xfcnebhqx']),
    ('mid', 0.04, 15000.0, True, ['Lady_Fern_wdvlditia', 'Common_Hemp_Nettle_xh1kdfbqx']),
    ('shrubs', 0.01, 30000.0, True, ['Raspberry_wf0oefeja']),
]
TAG = 'DensityTest'

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()


def meshes(folder):
    return [unreal.load_asset(str(d.package_name)) for d in registry.get_assets_by_path(MS + folder, recursive=True)
            if str(d.asset_class_path.asset_name) == 'StaticMesh']


def main():
    millions = float(sys.argv[1]) if len(sys.argv) > 1 else 1.0
    total = int(millions * 1000000)
    for a in actors.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            actors.destroy_actor(a)
    rng = random.Random(int(millions * 1000))
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    t0 = time.time()
    report = {'millions': millions, 'layers': {}}
    for name, share, cull, shadow, folders in LAYERS:
        ms = [m for f in folders for m in meshes(f)]
        actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
        actor.set_actor_label('Density_' + name)
        actor.tags = [TAG]
        root = sub.k2_gather_subobject_data_for_instance(actor)[0]
        comps = []
        for m in ms:
            handle, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(
                parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent, blueprint_context=None))
            c = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
            c.set_static_mesh(m)
            c.set_cull_distances(0, cull)
            c.set_cast_shadow(shadow)
            c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            comps.append(c)
        count = int(total * share)
        per = count // len(comps)
        for c in comps:
            left = per
            while left > 0:
                k = min(BATCH, left)
                xs = [unreal.Transform(unreal.Vector(rng.uniform(-CORE, CORE), rng.uniform(-CORE, CORE), 0.0),
                                       unreal.Rotator(roll=0.0, pitch=0.0, yaw=rng.uniform(0, 360)),
                                       unreal.Vector(*([rng.uniform(0.75, 1.25)] * 3))) for _ in range(k)]
                c.add_instances(xs, False, True)
                left -= k
        report['layers'][name] = {'instances': per * len(comps), 'meshes': len(comps), 'cull_cm': cull}
    report['instances'] = sum(l['instances'] for l in report['layers'].values())
    report['per_km2'] = report['instances']
    report['seconds'] = round(time.time() - t0, 1)
    with open(os.path.join(unreal.Paths.project_saved_dir(), 'density_{}.json'.format(sys.argv[1] if len(sys.argv) > 1 else '1')), 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[Density] ' + json.dumps({k: v for k, v in report.items() if k != 'layers'}))


main()
