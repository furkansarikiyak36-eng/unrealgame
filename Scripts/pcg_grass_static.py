"""Makes the PCG grass static for good: bEvaluateWorldPositionOffset=False in every Grass Spawner entry of
/Game/PerfTest/PCG_GrassScatter, saves the graph, regenerates and saves PerfTest.  py ".../pcg_grass_static.py" [on|off]
(off = static, the default). Report: Saved/pcg_grass_static.json"""
import json, os, sys
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
GRAPH = '/Game/PerfTest/PCG_GrassScatter'
wpo = (sys.argv[1] if len(sys.argv) > 1 else 'off') == 'on'
graph = unreal.load_asset(GRAPH)
changed, density = 0, None
for n in graph.get_editor_property('nodes'):
    s = n.get_settings()
    if isinstance(s, unreal.PCGSurfaceSamplerSettings):
        density = s.get_editor_property('points_per_squared_meter')
    if isinstance(s, unreal.PCGStaticMeshSpawnerSettings):
        params = s.get_editor_property('mesh_selector_parameters')
        entries = list(params.get_editor_property('mesh_entries'))
        for e in entries:
            d = e.get_editor_property('descriptor')
            d.set_editor_property('evaluate_world_position_offset', wpo)
            e.set_editor_property('descriptor', d)
            changed += 1
        params.set_editor_property('mesh_entries', entries)
unreal.EditorAssetLibrary.save_loaded_asset(graph, False)
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for c in a.get_components_by_class(unreal.PCGComponent):
        if c.get_graph() and c.get_graph().get_path_name().startswith(GRAPH + '.'):
            c.cleanup(True); c.generate(True)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(w, '/Game/Maps/PerfTest')
json.dump({'wpo': wpo, 'entries': changed, 'density': density}, open(os.path.join(unreal.Paths.project_saved_dir(), 'pcg_grass_static.json'), 'w'))
