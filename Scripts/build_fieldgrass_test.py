"""Grass performance demo in /Game/Maps/PerfTest: KiteDemo SM_FieldGrass_01 scattered by PCG over a SIZE_M x SIZE_M
square at the origin, DENSITY per m2. py ".../build_fieldgrass_test.py" [size_m=60] [density=4]
- /Game/PerfTest/PCG_FieldGrass: a copy of PCG_GrassScatter (the Kikuyu test graph is kept) with one mesh entry
  (FieldGrass, same descriptor template) and the new density.
- An existing GrassScatterVolume is removed; a new PCGVolume labelled GrassScatterVolume (so pcg_grass_shadow/wpo/cull
  toggles work on it) is spawned, generated, and the map saved. Report: Saved/fieldgrass_test.json (instances)."""
import json, os, sys
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
SIZE_M = float(sys.argv[1]) if len(sys.argv) > 1 else 60.0
DENSITY = float(sys.argv[2]) if len(sys.argv) > 2 else 4.0
SRC, GRAPH = '/Game/PerfTest/PCG_GrassScatter', '/Game/PerfTest/PCG_FieldGrass'
MESH = '/Game/KiteDemo/Environments/Foliage/Grass/FieldGrass/SM_FieldGrass_01.SM_FieldGrass_01'
lib = unreal.EditorAssetLibrary
if not lib.does_asset_exist(GRAPH):
    lib.duplicate_asset(SRC, GRAPH)
graph = unreal.load_asset(GRAPH)
for n in graph.get_editor_property('nodes'):
    s = n.get_settings()
    if isinstance(s, unreal.PCGSurfaceSamplerSettings):
        s.set_editor_property('points_per_squared_meter', DENSITY)
    if isinstance(s, unreal.PCGStaticMeshSpawnerSettings):
        params = s.get_editor_property('mesh_selector_parameters')
        old = list(params.get_editor_property('mesh_entries'))
        tpl = old[0].export_text()
        old_path = str(old[0].get_editor_property('descriptor').get_editor_property('static_mesh').get_path_name())
        e = unreal.PCGMeshSelectorWeightedEntry(); e.import_text(tpl.replace(old_path, MESH)); e.set_editor_property('weight', 1)
        params.set_editor_property('mesh_entries', [e])
lib.save_loaded_asset(graph, False)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in actors.get_all_level_actors():
    if a.get_actor_label() == 'GrassScatterVolume' or a.get_class().get_name() == 'PCGPartitionActor':
        actors.destroy_actor(a)
vol = actors.spawn_actor_from_class(unreal.PCGVolume, unreal.Vector(0, 0, 0))
vol.set_actor_label('GrassScatterVolume')
vol.set_actor_scale3d(unreal.Vector(SIZE_M / 2.0, SIZE_M / 2.0, 20.0))   # default brush is 200 cm
comp = vol.get_component_by_class(unreal.PCGComponent)
try:
    comp.set_graph(graph)
except Exception:
    comp.set_editor_property('graph_instance', None); comp.set_graph_local(graph)
comp.generate(True)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
inst = 0
for a in actors.get_all_level_actors():
    for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        m = c.get_editor_property('static_mesh')
        if m and m.get_name() == 'SM_FieldGrass_01':
            inst += c.get_instance_count()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
o, e = vol.get_actor_bounds(False)
json.dump({'size_m': SIZE_M, 'density': DENSITY, 'instances': inst, 'expected': int(SIZE_M * SIZE_M * DENSITY),
           'volume_extent_m': [round(e.x / 100, 1), round(e.y / 100, 1)]},
          open(os.path.join(unreal.Paths.project_saved_dir(), 'fieldgrass_test.json'), 'w'), indent=1)
