"""Sets points_per_squared_meter of the surface sampler in /Game/PerfTest/PCG_GrassScatter and regenerates.
py ".../pcg_grass_density.py" <points per m2>   Report: Saved/pcg_grass_density.json"""
import json, os, sys
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
GRAPH = '/Game/PerfTest/PCG_GrassScatter'
value = float(sys.argv[1])
graph = unreal.load_asset(GRAPH)
old = None
for n in graph.get_editor_property('nodes'):
    s = n.get_settings()
    if isinstance(s, unreal.PCGSurfaceSamplerSettings):
        old = s.get_editor_property('points_per_squared_meter')
        s.set_editor_property('points_per_squared_meter', value)
unreal.EditorAssetLibrary.save_loaded_asset(graph, False)
regen = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for c in a.get_components_by_class(unreal.PCGComponent):
        if c.get_graph() and c.get_graph().get_path_name().startswith(GRAPH + '.'):
            c.cleanup(True); c.generate(True); regen += 1
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(w, '/Game/Maps/PerfTest')
json.dump({'old': old, 'new': value, 'regenerated': regen}, open(os.path.join(unreal.Paths.project_saved_dir(), 'pcg_grass_density.json'), 'w'))
