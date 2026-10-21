"""Saves the open map if it has a name, then opens: py ".../switch_map.py" blank|template|<asset path>
blank = new unsaved empty map, template = new unsaved map from Template_Default (sky, sun, fog, floor).
Report: Saved/switch_map.json"""
import json, os, sys
import unreal
target = sys.argv[1] if len(sys.argv) > 1 else 'blank'
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
path = w.get_path_name().split('.')[0]
if path.startswith('/Game/'):
    unreal.EditorLoadingAndSavingUtils.save_map(w, path)
if target == 'blank':
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
elif target == 'template':
    unreal.EditorLoadingAndSavingUtils.new_map_from_template('/Engine/Maps/Templates/Template_Default', False)
else:
    unreal.EditorLoadingAndSavingUtils.load_map(target)
w = ues.get_editor_world()
unreal.SystemLibrary.collect_garbage()
json.dump({'from': path, 'now': w.get_path_name(), 'actors': len(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())},
          open(os.path.join(unreal.Paths.project_saved_dir(), 'switch_map.json'), 'w'))
