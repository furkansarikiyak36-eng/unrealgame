"""Second heavy-asset deletion, approved by the user on 2026-10-19 ("KiteDemo trees excepted" + "delete the used ones too").

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/delete_heavy_round2.py"
Deletes every package in Saved/heavy_delete_round2.json (files copied to Saved/Backups/2026-10-19/deleted_heavy_2 first).
Assets still referenced by maps are deleted as well; the maps then show the objects as missing. The open level is saved.
Report: Saved/delete_heavy_round2.json
"""
import json
import os

import unreal

SAVED = unreal.Paths.project_saved_dir()
lib = unreal.EditorAssetLibrary
report = {'deleted': [], 'failed': []}
for pkg in json.load(open(os.path.join(SAVED, 'heavy_delete_round2.json'))):
    try:
        if lib.does_asset_exist(pkg) and lib.delete_asset(pkg):
            report['deleted'].append(pkg)
        else:
            report['failed'].append(pkg)
    except Exception as e:
        report['failed'].append(pkg + ' : ' + str(e)[:150])
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
path = world.get_path_name().split('.')[0]
if path.startswith('/Game/'):
    unreal.EditorLoadingAndSavingUtils.save_map(world, path)
report['saved_map'] = path
json.dump(report, open(os.path.join(SAVED, 'delete_heavy_round2.json'), 'w'), indent=1)
unreal.log('[DeleteHeavy2] deleted {} failed {}'.format(len(report['deleted']), len(report['failed'])))
