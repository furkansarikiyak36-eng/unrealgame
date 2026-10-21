"""Dense natural vegetation in /Game/Maps/PerfTest: Megascans plants and Fab rocks in a 1 x 1 km core around the
origin, laid out like nature rather than uniformly. Removes the uniform PerfTest grass/plants/stones layers first.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/build_nature.py"
- grass: dense everywhere except in rock outcrops, each grass species in large soft patches (value noise);
- ground cover, mid plants, shrubs: species in clusters (cluster centres, gaussian spread), ferns and balsam favour
  the 'moist' noise, poppies and burdock the dry side;
- rocks: outcrops of 1-3 boulders with medium and small stones around them, sunk into the ground and tilted, plus
  loose small stones everywhere.
One actor per layer (tag NatureLayer_<name>), one HISM per mesh, per-layer cull distance. Seeded: re-running gives the
same scene. The ground's material is switched to Megascans Wild Grass. Saves the map. Report: Saved/nature.json.
"""
import json
import math
import os
import random
import time

import unreal

MAP = '/Game/Maps/PerfTest'
DEST = '/Game/PerfTest'
CORE = 50000.0                       # half side of the dense core: 1 km x 1 km
MS = '/Game/Fab/Megascans/'
BATCH = 50000

GRASS = ['Wild_Grass_vczndjqja', 'Kikuyu_Grass_wiljearqx', 'Dry_Grass_tbbqejqr', 'Thatching_Grass_uddmcgbia',
         'Wheat_Grass_tdcrdbur', 'Ribbon_Grass_tbdpec3r']
COVER = ['Wood_Sorrel_wcwmchzja', 'Violet_Wood_Sorrel_uchkajuia', 'Matted_Pratia_uegjcflia', 'English_Ivy_xfcnebhqx',
         'Lily_of_the_Valley_xikkdhjja', 'Ground_Cover_sfcnq', 'Curbside_Weeds_ujzjfexja']
MID_MOIST = ['Lady_Fern_wdvlditia', 'Beech_Fern_vmkpdbeia', 'Small_Balsam_xefmcgrqx', 'Yellow_Archangel_wcwmcfvia',
             'Common_Hemp_Nettle_xh1kdfbqx', 'Bog_Marshcress_theqfivr']
MID_DRY = ['Field_Poppy_vmcobd0ja', 'Greater_Burdock_xh5pdgiqx', 'Sage_ve3gbfoja', 'Dead_Shrubs_vdknafgha']
SHRUBS = ['Raspberry_wf0oefeja', 'Elderberry_wfzobb2ia', 'European_Spindle_wk1ncbxja', 'Black_Locust_wdvhberja']
BOULDERS = ['Mossy_Forest_Boulder_wfstcdnaw', 'Sandstone_Boulder_vmjjfdm', 'Beach_Boulder_ukopeess',
            'Beach_Boulder_ukopeijs', 'Beach_Boulder_wenobdn', 'Beach_Rock_wd3efb0', 'Rock_shopk']
STONES = ['Small_Stones_Pack_vcljbb1iw', 'Small_Stones_Pack_vdjkbfmiw', 'Mossy_Stones_Pack_tliiadmva',
          'Nordic_Beach_Rocks_ulznddxva', 'Military_Trenches_Scatter_Rock_S_10_ydxsabu', 'Nordic_Beach_Rock_vbumba2fa']

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()


class Noise:
    """Smooth 2D value noise, a few octaves, values about 0..1."""
    def __init__(self, seed, cell):
        self.rng = random.Random(seed)
        self.cell = cell
        self.table = {}

    def _v(self, ix, iy):
        key = (ix, iy)
        if key not in self.table:
            self.table[key] = self.rng.random()
        return self.table[key]

    def _smooth(self, x, y):
        ix, iy = math.floor(x), math.floor(y)
        fx, fy = x - ix, y - iy
        fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
        a = self._v(ix, iy) + (self._v(ix + 1, iy) - self._v(ix, iy)) * fx
        b = self._v(ix, iy + 1) + (self._v(ix + 1, iy + 1) - self._v(ix, iy + 1)) * fx
        return a + (b - a) * fy

    def __call__(self, x, y):
        x, y = x / self.cell, y / self.cell
        return (self._smooth(x, y) * 0.6 + self._smooth(x * 2.1, y * 2.1) * 0.3 + self._smooth(x * 4.3, y * 4.3) * 0.1)


def meshes(folders, sub='Plants'):
    out = {}
    for f in folders:
        out[f] = [unreal.load_asset(str(d.package_name))
                  for d in registry.get_assets_by_path(MS + sub + '/' + f, recursive=True)
                  if str(d.asset_class_path.asset_name) == 'StaticMesh']
    return out


def clear(tags):
    for a in actors.get_all_level_actors():
        if any(t in [str(x) for x in a.tags] for t in tags):
            actors.destroy_actor(a)


def layer_actor(name, buckets, cull):
    """buckets: {mesh: [Transform, ...]} -> one actor with one HISM per mesh."""
    tag = 'NatureLayer_' + name
    actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    actor.set_actor_label(tag)
    actor.tags = [tag]
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    root = sub.k2_gather_subobject_data_for_instance(actor)[0]
    total = 0
    for mesh, xforms in buckets.items():
        if not xforms:
            continue
        handle, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent, blueprint_context=None))
        comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
        comp.set_static_mesh(mesh)
        if cull:
            comp.set_cull_distances(0, cull)
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        for i in range(0, len(xforms), BATCH):
            comp.add_instances(xforms[i:i + BATCH], False, True)
        total += len(xforms)
    return total


def xform(x, y, z, yaw, scale, tilt=0.0, rng=None):
    pitch = rng.uniform(-tilt, tilt) if tilt else 0.0
    roll = rng.uniform(-tilt, tilt) if tilt else 0.0
    return unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), unreal.Vector(scale, scale, scale))


def main():
    t0 = time.time()
    clear(['PerfLayer_grass', 'PerfLayer_plants', 'PerfLayer_stones', 'NatureLayer_grass', 'NatureLayer_cover',
           'NatureLayer_mid', 'NatureLayer_shrubs', 'NatureLayer_rocks'])
    rng = random.Random('nature')
    moist = Noise('moist', 9000.0)       # ferns, balsam vs poppy, burdock
    patch = Noise('patch', 2500.0)       # which grass species
    meadow = Noise('meadow', 4000.0)     # grass density

    # Rock outcrops first: grass and plants thin out on them.
    boulder_m = [m for ms in meshes(BOULDERS, '3D').values() for m in ms]
    stone_m = [m for ms in meshes(STONES, '3D').values() for m in ms]
    rocks = {m: [] for m in boulder_m + stone_m}
    outcrops = []
    for _ in range(150):
        cx, cy = rng.uniform(-CORE, CORE), rng.uniform(-CORE, CORE)
        radius = rng.uniform(300, 1200)
        outcrops.append((cx, cy, radius))
        for _ in range(rng.randint(1, 3)):
            m = rng.choice(boulder_m)
            s = rng.uniform(0.8, 1.8)
            h = (m.get_bounding_box().max.z - m.get_bounding_box().min.z) * s
            rocks[m].append(xform(cx + rng.gauss(0, radius * 0.3), cy + rng.gauss(0, radius * 0.3), -h * rng.uniform(0.1, 0.3),
                                  rng.uniform(0, 360), s, 15, rng))
        for _ in range(rng.randint(10, 30)):
            m = rng.choice(stone_m)
            s = rng.uniform(0.6, 2.2)
            rocks[m].append(xform(cx + rng.gauss(0, radius * 0.7), cy + rng.gauss(0, radius * 0.7), -2.0, rng.uniform(0, 360), s, 30, rng))
    for _ in range(10000):                       # loose stones everywhere
        m = rng.choice(stone_m)
        rocks[m].append(xform(rng.uniform(-CORE, CORE), rng.uniform(-CORE, CORE), -1.0, rng.uniform(0, 360), rng.uniform(0.5, 1.4), 30, rng))

    def rocky(x, y):
        for cx, cy, r in outcrops:
            if abs(x - cx) < r and abs(y - cy) < r and (x - cx) ** 2 + (y - cy) ** 2 < (r * 0.8) ** 2:
                return True
        return False

    # Index outcrops on a coarse grid so the 'rocky' test stays cheap for millions of points.
    grid = {}
    for o in outcrops:
        for gx in range(int((o[0] - o[2]) // 2000), int((o[0] + o[2]) // 2000) + 1):
            for gy in range(int((o[1] - o[2]) // 2000), int((o[1] + o[2]) // 2000) + 1):
                grid.setdefault((gx, gy), []).append(o)

    def on_rock(x, y):
        for cx, cy, r in grid.get((int(x // 2000), int(y // 2000)), ()):
            if (x - cx) ** 2 + (y - cy) ** 2 < (r * 0.8) ** 2:
                return True
        return False

    # Grass: ~2.5 candidates per m2, kept by the meadow noise, species by the patch noise.
    grass_sets = meshes(GRASS)
    grass = {m: [] for ms in grass_sets.values() for m in ms}
    names = list(grass_sets)
    for _ in range(2500000):
        x, y = rng.uniform(-CORE, CORE), rng.uniform(-CORE, CORE)
        if rng.random() > 0.35 + 0.65 * meadow(x, y) or on_rock(x, y):
            continue
        species = names[min(len(names) - 1, int(patch(x, y) * len(names) * 1.2) % len(names))]
        m = rng.choice(grass_sets[species])
        grass[m].append(xform(x, y, 0.0, rng.uniform(0, 360), rng.uniform(0.7, 1.3)))

    def clusters(folders_moist, folders_dry, count, per, spread, scale, layer_meshes):
        sets = meshes(folders_moist + folders_dry)
        out = {m: [] for ms in sets.values() for m in ms}
        placed = 0
        while placed < count:
            cx, cy = rng.uniform(-CORE, CORE), rng.uniform(-CORE, CORE)
            wet = moist(cx, cy)
            pool = folders_moist if (wet > 0.5 or not folders_dry) else folders_dry
            if folders_moist and folders_dry and rng.random() < 0.15:
                pool = folders_moist + folders_dry
            species = rng.choice(pool)
            n = rng.randint(per // 2, per)
            for _ in range(n):
                x, y = cx + rng.gauss(0, spread), cy + rng.gauss(0, spread)
                if abs(x) > CORE or abs(y) > CORE or on_rock(x, y):
                    continue
                m = rng.choice(sets[species])
                out[m].append(xform(x, y, 0.0, rng.uniform(0, 360), rng.uniform(*scale)))
                placed += 1
        return out

    cover = clusters(COVER, [], 150000, 60, 250.0, (0.8, 1.3), None)
    mid = clusters(MID_MOIST, MID_DRY, 80000, 40, 350.0, (0.8, 1.25), None)
    shrubs = clusters(SHRUBS[:2] + SHRUBS[3:], SHRUBS[2:3], 10000, 8, 400.0, (0.8, 1.2), None)

    counts = {
        'grass': layer_actor('grass', grass, 8000.0),
        'cover': layer_actor('cover', cover, 10000.0),
        'mid': layer_actor('mid', mid, 20000.0),
        'shrubs': layer_actor('shrubs', shrubs, 40000.0),
        'rocks': layer_actor('rocks', rocks, 0.0),
    }
    # Ground: Megascans Wild Grass surface, 2 m per repeat over the 3.5 km cube top.
    mi = unreal.load_asset(DEST + '/MI_PerfGround')
    unreal.MaterialEditingLibrary.set_material_instance_parent(
        mi, unreal.load_asset(MS + 'Surfaces/Wild_Grass_sfknaeoa/Low/sfknaeoa_tier_3/Materials/MI_sfknaeoa'))
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, 'Tiling', 1750.0)
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    report = {'core_km2': (CORE * 2 / 100000.0) ** 2, 'counts': counts, 'outcrops': len(outcrops),
              'meshes': {'grass': len(grass), 'cover': len(cover), 'mid': len(mid), 'shrubs': len(shrubs), 'rocks': len(rocks)},
              'seconds': round(time.time() - t0, 1)}
    with open(os.path.join(unreal.Paths.project_saved_dir(), 'nature.json'), 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[Nature] ' + json.dumps(report))


main()
