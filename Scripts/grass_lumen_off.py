"""Keeps the PCG grass out of the Lumen scene and distance-field lighting (Lumen was re-capturing grass surface cache
cards every frame: LumenSceneUpdate 46 ms of a 78 ms PIE frame). Sets bAffectDynamicIndirectLighting and
bAffectDistanceFieldLighting to False in every mesh entry of the given PCG graph (permanent), and on the ISM components
of GrassScatterVolume now; saves graph and map.
py ".../grass_lumen_off.py" [graph=/Game/PerfTest/PCG_FieldGrass] -> Saved/grass_lumen_off.json"""
import json, os, sys
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
GRAPH = sys.argv[1] if len(sys.argv) > 1 else '/Game/PerfTest/PCG_FieldGrass'
graph = unreal.load_asset(GRAPH)
rep = {'entries': 0, 'components': 0, 'descriptor_fields': []}
for n in graph.get_editor_property('nodes'):
    s = n.get_settings()
    if not isinstance(s, unreal.PCGStaticMeshSpawnerSettings):
        continue
    params = s.get_editor_property('mesh_selector_parameters')
    entries = list(params.get_editor_property('mesh_entries'))
    for e in entries:
        d = e.get_editor_property('descriptor')
        for f in ('affect_dynamic_indirect_lighting', 'affect_distance_field_lighting'):
            try:
                d.set_editor_property(f, False); rep['descriptor_fields'].append(f)
            except Exception as ex:
                rep.setdefault('errors', []).append(f + ': ' + str(ex)[:80])
        e.set_editor_property('descriptor', d)
        rep['entries'] += 1
    params.set_editor_property('mesh_entries', entries)
unreal.EditorAssetLibrary.save_loaded_asset(graph, False)
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == 'GrassScatterVolume' or a.get_class().get_name() == 'PCGPartitionActor':
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            c.set_editor_property('affect_dynamic_indirect_lighting', False)
            c.set_editor_property('affect_distance_field_lighting', False)
            c.mark_render_state_dirty() if hasattr(c, 'mark_render_state_dirty') else None
            rep['components'] += 1
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
rep['descriptor_fields'] = sorted(set(rep['descriptor_fields']))
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'grass_lumen_off.json'), 'w'), indent=1)
