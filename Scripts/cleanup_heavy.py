"""Heavy-asset clean-up approved by the user on 2026-10-19.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/cleanup_heavy.py"
1. Deletes the packages in Saved/heavy_safe_delete.json (heavy, used by no map, referenced by nothing that stays).
   Their files were copied to Saved/Backups/2026-10-19/deleted_heavy first.
2. Caps every texture in Saved/heavy_textures_cap.json at 2048 px (Max Texture Size). Nothing is deleted; setting the
   value back to 0 restores the full size.
Report: Saved/cleanup_heavy.json
"""
import json
import os

import unreal

SAVED = unreal.Paths.project_saved_dir()
lib = unreal.EditorAssetLibrary
report = {'deleted': [], 'delete_failed': [], 'capped': [], 'cap_failed': []}

for pkg in json.load(open(os.path.join(SAVED, 'heavy_safe_delete.json'))):
    if lib.does_asset_exist(pkg) and lib.delete_asset(pkg):
        report['deleted'].append(pkg)
    else:
        report['delete_failed'].append(pkg)

with unreal.ScopedSlowTask(1, 'Capping heavy textures at 2048') as task:
    for pkg in json.load(open(os.path.join(SAVED, 'heavy_textures_cap.json'))):
        try:
            tex = unreal.load_asset(pkg)
            before = tex.get_editor_property('max_texture_size')
            tex.set_editor_property('max_texture_size', 2048)
            lib.save_loaded_asset(tex, False)
            report['capped'].append({'texture': pkg, 'before': before})
        except Exception as e:
            report['cap_failed'].append({'texture': pkg, 'error': str(e)[:200]})

json.dump(report, open(os.path.join(SAVED, 'cleanup_heavy.json'), 'w'), indent=1)
unreal.log('[CleanupHeavy] deleted {} (failed {}), capped {} (failed {})'.format(
    len(report['deleted']), len(report['delete_failed']), len(report['capped']), len(report['cap_failed'])))
