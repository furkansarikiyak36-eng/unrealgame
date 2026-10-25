"""Turns Nanite on for the heavy static meshes of KiteDemo and ParagonProps (LOD0 at least MIN_TRIS triangles).

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/enable_nanite_kits.py" [min_tris]
Memory-safe and resumable like cap_kit_textures.py: one mesh at a time, garbage collection every BATCH meshes, each
change appended at once to Saved/nanite_kits.jsonl ([package, triangles]); meshes already Nanite are skipped.
To undo: turn Nanite off on the meshes listed there. Summary: Saved/nanite_kits.json
"""
import json
import os
import sys

import unreal

MIN_TRIS = int(sys.argv[1]) if len(sys.argv) > 1 else 10000
BATCH = 5
SAVED = unreal.Paths.project_saved_dir()
LOG = os.path.join(SAVED, 'nanite_kits.jsonl')
reg = unreal.AssetRegistryHelpers.get_asset_registry()
sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

todo = []
for kit in ('KiteDemo', 'ParagonProps'):
    for d in reg.get_assets_by_path('/Game/' + kit, recursive=True):
        if str(d.asset_class_path.asset_name) != 'StaticMesh':
            continue
        try:
            tris = int(float(str(d.get_tag_value('Triangles') or 0)))
        except ValueError:
            tris = 0
        if tris >= MIN_TRIS and str(d.get_tag_value('NaniteEnabled')) != 'True':
            todo.append((str(d.package_name), tris))

done = 0
with unreal.ScopedSlowTask(len(todo), 'Enabling Nanite') as task:
    task.make_dialog(True)
    for i, (pkg, tris) in enumerate(todo):
        if task.should_cancel():
            break
        task.enter_progress_frame(1, pkg.split('/')[-1])
        mesh = unreal.load_asset(pkg)
        settings = mesh.get_editor_property('nanite_settings')
        if not settings.enabled:
            settings.enabled = True
            sm.set_nanite_settings(mesh, settings, True)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
            with open(LOG, 'a') as f:
                f.write(json.dumps([pkg, tris]) + '\n')
            done += 1
        mesh = None
        if (i + 1) % BATCH == 0:
            unreal.SystemLibrary.collect_garbage()
unreal.SystemLibrary.collect_garbage()
json.dump({'candidates': len(todo), 'enabled': done, 'min_tris': MIN_TRIS}, open(os.path.join(SAVED, 'nanite_kits.json'), 'w'))
unreal.log('[NaniteKits] candidates {} enabled {}'.format(len(todo), done))
