"""Dense nature in /Game/Maps/PerfTest with PCG instead of hand-placed instances.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/build_pcg_nature.py"
Builds the graph /Game/PerfTest/PCG_DenseNature and 10 x 10 PCG Volumes (100 m tiles) over the 1 x 1 km core,
then generates them.
Each layer is one branch:  Volume Sampler (voxel = spacing) -> Transform Points (jitter inside the voxel, yaw,
scale) -> Spatial Noise (writes Density) -> Density Filter (keeps the patches) -> Static Mesh Spawner (weighted
Megascans meshes; cull distance, shadows and NoCollision set on the instance descriptor).
The volume is 1 m thick around z = 0 so the samplers put one layer of points on the flat ground.
Removes the hand-placed NatureLayer_* actors and an earlier PCG volume (tag PCGNature). Generation runs
asynchronously; Scripts/pcg_status.py reports what it spawned. Report: Saved/build_pcg_nature.json.
"""
import json
import os

import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))

DEST = '/Game/PerfTest'
GRAPH = DEST + '/PCG_DenseNature'
CORE = 50000.0
MS = '/Game/Fab/Megascans/'
TAG = 'PCGNature'
TILES = 10                           # 10 x 10 volumes of 100 m

# layer: spacing cm, density filter lower bound, noise scale (smaller = larger patches), cull cm, shadow, scale range,
#        z offset range, folders with weights.
LAYERS = [
    ('grass', 45.0, 0.30, 0.0004, 5000.0, False, (0.7, 1.3), (0.0, 0.0),
     {'Wild_Grass_vczndjqja': 3, 'Kikuyu_Grass_wiljearqx': 3, 'Ribbon_Grass_tbdpec3r': 2, 'Wheat_Grass_tdcrdbur': 2,
      'Dry_Grass_tbbqejqr': 1, 'Thatching_Grass_uddmcgbia': 1}, '3D_PLANTS'),
    ('cover', 90.0, 0.45, 0.0008, 8000.0, False, (0.8, 1.3), (0.0, 0.0),
     {'Wood_Sorrel_wcwmchzja': 2, 'Violet_Wood_Sorrel_uchkajuia': 2, 'Matted_Pratia_uegjcflia': 1, 'English_Ivy_xfcnebhqx': 2,
      'Lily_of_the_Valley_xikkdhjja': 1, 'Ground_Cover_sfcnq': 2}, '3D_PLANTS'),
    ('mid', 200.0, 0.50, 0.0006, 15000.0, True, (0.8, 1.25), (0.0, 0.0),
     {'Lady_Fern_wdvlditia': 3, 'Beech_Fern_vmkpdbeia': 3, 'Small_Balsam_xefmcgrqx': 2, 'Yellow_Archangel_wcwmcfvia': 2,
      'Common_Hemp_Nettle_xh1kdfbqx': 2, 'Field_Poppy_vmcobd0ja': 1, 'Greater_Burdock_xh5pdgiqx': 1, 'Sage_ve3gbfoja': 1}, '3D_PLANTS'),
    ('shrubs', 600.0, 0.55, 0.0005, 30000.0, True, (0.8, 1.2), (0.0, 0.0),
     {'Raspberry_wf0oefeja': 3, 'Elderberry_wfzobb2ia': 2, 'European_Spindle_wk1ncbxja': 1, 'Black_Locust_wdvhberja': 1,
      'Dead_Shrubs_vdknafgha': 2}, '3D_PLANTS'),
    ('boulders', 1500.0, 0.62, 0.0003, 0.0, True, (0.8, 2.0), (-60.0, -15.0),
     {'Mossy_Forest_Boulder_wfstcdnaw': 2, 'Sandstone_Boulder_vmjjfdm': 1, 'Beach_Boulder_ukopeess': 1,
      'Beach_Boulder_wenobdn': 1, 'Beach_Rock_wd3efb0': 1, 'Rock_shopk': 1}, '3D'),
    ('stones', 300.0, 0.55, 0.0003, 20000.0, True, (0.6, 1.6), (-3.0, 0.0),
     {'Small_Stones_Pack_vcljbb1iw': 2, 'Small_Stones_Pack_vdjkbfmiw': 2, 'Mossy_Stones_Pack_tliiadmva': 2,
      'Nordic_Beach_Rocks_ulznddxva': 1, 'Military_Trenches_Scatter_Rock_S_10_ydxsabu': 1}, '3D'),
]

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()


def meshes(folder, kind):
    root = MS + ('Plants/' if kind == '3D_PLANTS' else '3D/') + folder
    return [str(d.package_name) + '.' + str(d.asset_name) for d in registry.get_assets_by_path(root, recursive=True)
            if str(d.asset_class_path.asset_name) == 'StaticMesh']


def entry(mesh_path, weight, cull, shadow):
    e = unreal.PCGMeshSelectorWeightedEntry()
    e.import_text('(Descriptor=(StaticMesh="{}",InstanceEndCullDistance={},CastShadow={},'
                  'BodyInstance=(CollisionEnabled=NoCollision,CollisionProfileName="NoCollision")),Weight={})'
                  .format(mesh_path, int(cull), 'True' if shadow else 'False', weight))
    return e


def add(graph, cls):
    result = graph.add_node_of_type(cls)
    return result if isinstance(result, tuple) else (result, result.get_settings())


def build_graph():
    if unreal.EditorAssetLibrary.does_asset_exist(GRAPH):
        unreal.EditorAssetLibrary.delete_asset(GRAPH)
    graph = unreal.AssetToolsHelpers.get_asset_tools().create_asset('PCG_DenseNature', DEST, unreal.PCGGraph,
                                                                    unreal.PCGGraphFactory())
    inp, out = graph.get_input_node(), graph.get_output_node()
    report = {}
    for i, (name, spacing, lower, nscale, cull, shadow, scale, zoff, folders, kind) in enumerate(LAYERS):
        sampler, s = add(graph, unreal.PCGVolumeSamplerSettings)
        s.set_editor_property('voxel_size', unreal.Vector(spacing, spacing, 100.0))
        xf, t = add(graph, unreal.PCGTransformPointsSettings)
        half = spacing * 0.5
        t.set_editor_property('offset_min', unreal.Vector(-half, -half, zoff[0]))
        t.set_editor_property('offset_max', unreal.Vector(half, half, zoff[1]))
        t.set_editor_property('rotation_min', unreal.Rotator(0, 0, 0))
        t.set_editor_property('rotation_max', unreal.Rotator(roll=0.0, pitch=0.0, yaw=360.0) if kind == '3D_PLANTS' else unreal.Rotator(roll=15.0, pitch=15.0, yaw=360.0))
        t.set_editor_property('uniform_scale', True)
        t.set_editor_property('scale_min', unreal.Vector(scale[0], scale[0], scale[0]))
        t.set_editor_property('scale_max', unreal.Vector(scale[1], scale[1], scale[1]))
        t.set_editor_property('seed', 101 + i)
        noise, n = add(graph, unreal.PCGSpatialNoiseSettings)
        n.set_editor_property('mode', unreal.PCGSpatialNoiseMode.PERLIN2D)
        n.set_editor_property('iterations', 4)
        n.set_editor_property('transform', unreal.Transform(unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0),
                                                            unreal.Vector(nscale, nscale, nscale)))
        n.set_editor_property('seed', 7 + i * 13)
        filt, f = add(graph, unreal.PCGDensityFilterSettings)
        f.set_editor_property('lower_bound', lower)
        f.set_editor_property('upper_bound', 1.0)
        spawn, sp = add(graph, unreal.PCGStaticMeshSpawnerSettings)
        sp.set_mesh_selector_type(unreal.PCGMeshSelectorWeighted)
        entries = []
        for folder, w in folders.items():
            ms = meshes(folder, kind)
            for m in ms:
                entries.append(entry(m, max(1, int(round(w * 10.0 / len(ms)))), cull, shadow))
        sp.get_editor_property('mesh_selector_parameters').set_editor_property('mesh_entries', entries)
        graph.add_edge(inp, 'In', sampler, 'Volume')
        graph.add_edge(sampler, 'Out', xf, 'In')
        graph.add_edge(xf, 'Out', noise, 'In')
        graph.add_edge(noise, 'Out', filt, 'In')
        graph.add_edge(filt, 'Out', spawn, 'In')
        graph.add_edge(spawn, 'Out', out, 'Out')
        report[name] = {'spacing_cm': spacing, 'points_per_m2_before_filter': round(10000.0 / spacing ** 2, 2),
                        'keep_density_from': lower, 'meshes': len(entries), 'cull_cm': cull}
    unreal.EditorAssetLibrary.save_loaded_asset(graph, False)
    return graph, report


def main():
    for a in actors.get_all_level_actors():
        tags = [str(t) for t in a.tags]
        if TAG in tags or any(t.startswith('NatureLayer_') for t in tags):
            actors.destroy_actor(a)
    graph, report = build_graph()
    # One volume per 100 m tile: a single Volume Sampler is capped at about a million points, and per-tile
    # components cull as a whole when far away. The noise is in world space, so patches run on across tiles;
    # each tile has its own seed so the jitter does not repeat.
    step = CORE * 2 / TILES
    for ix in range(TILES):
        for iy in range(TILES):
            cx, cy = -CORE + step * (ix + 0.5), -CORE + step * (iy + 0.5)
            volume = actors.spawn_actor_from_class(unreal.PCGVolume, unreal.Vector(cx, cy, 0))
            volume.set_actor_label('PCG_DenseNature_{}_{}'.format(ix, iy))
            volume.set_folder_path('PCGNature')
            volume.tags = [TAG]
            volume.set_actor_scale3d(unreal.Vector(step / 200.0, step / 200.0, 0.5))
            comp = volume.get_component_by_class(unreal.PCGComponent)
            comp.set_graph(graph)
            comp.set_editor_property('seed', 1000 + ix * TILES + iy)
            comp.generate(True)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
    report = {'graph': GRAPH, 'volume_km2': (CORE * 2 / 100000.0) ** 2, 'layers': report}
    with open(os.path.join(unreal.Paths.project_saved_dir(), 'build_pcg_nature.json'), 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[PCGNature] graph built, generation started')


main()
