"""Read only: collision of the imported UnrealExport meshes. Writes Saved/probe_collision.json."""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'probe_collision.json')
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
rep = {}
for name in ('SM_RawLog_150', 'SM_RawLog_150_Trough', 'Archive/SM_RawLog_75_Notch_Saddle'):
    m = unreal.load_asset('/Game/UnrealExport/' + name)
    body = m.get_editor_property('body_setup')
    rep[name] = {'simple': sub.get_simple_collision_count(m), 'convex': sub.get_convex_collision_count(m),
                 'complexity': str(body.get_editor_property('collision_trace_flag')) if body else None,
                 'body_api': sorted(n for n in dir(body) if 'geom' in n.lower() or 'collision' in n.lower())[:20] if body else None}
rep['interchange'] = str(unreal.SystemLibrary.get_console_variable_int_value('Interchange.FeatureFlags.Import.FBX'))
json.dump(rep, open(OUT, 'w'), indent=1)
unreal.log('[ProbeCollision] ' + json.dumps(rep))
