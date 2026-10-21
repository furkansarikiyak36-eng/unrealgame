"""PerfTest plays with the project's own character: World Settings game mode override -> AForestGameMode (was the
third-person test mode). py ".../set_perftest_forest_mode.py" [tp] (tp: back to BP_TP_GameMode) -> Saved/perftest_mode.json"""
import json, os, sys
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first; open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
ws = _OPEN.get_world_settings()
old = ws.get_editor_property('default_game_mode')
if len(sys.argv) > 1 and sys.argv[1] == 'tp':
    new = unreal.load_object(None, '/Game/PerfTest/ThirdPerson/BP_TP_GameMode.BP_TP_GameMode_C')
else:
    new = unreal.ForestGameMode.static_class()
ws.set_editor_property('default_game_mode', new)
unreal.EditorLoadingAndSavingUtils.save_map(_OPEN, '/Game/Maps/PerfTest')
json.dump({'old': old.get_path_name() if old else None, 'new': new.get_path_name() if new else None},
          open(os.path.join(unreal.Paths.project_saved_dir(), 'perftest_mode.json'), 'w'), indent=1)
