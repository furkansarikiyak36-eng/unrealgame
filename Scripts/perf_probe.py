"""Read only: is Main dirty, and the csvprofile output folder. Writes Saved/perf_probe.json."""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'perf_probe.json')
rep = {'dirty_maps': [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()],
       'dirty_content': [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()][:20],
       'level': unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name()}
json.dump(rep, open(OUT, 'w'), indent=1)
unreal.log('[PerfProbe] ' + json.dumps(rep))
