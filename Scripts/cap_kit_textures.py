"""Caps the textures of KiteDemo, ParagonProps and SoulCave: anything over 2048 px to 2048, uncompressed HDR (FloatRGBA)
over 1024 px to 1024. Nothing is deleted.  py ".../cap_kit_textures.py"

Memory-safe and resumable: textures are handled one by one, garbage is collected every BATCH textures (the first run
kept ~340 4K/8K sources in memory and the editor ran out of memory), and every change is appended to
Saved/kit_texture_caps.jsonl at once ([package, dims, format, previous Max Texture Size, new]). Textures already at or
under the limit are skipped, so a re-run continues where the last one stopped. Summary: Saved/kit_texture_caps.json.
To undo: set Max Texture Size back to the previous value (0 = no limit) for the rows in the .jsonl file.
"""
import json
import os

import unreal

BATCH = 10
SAVED = unreal.Paths.project_saved_dir()
LOG = os.path.join(SAVED, 'kit_texture_caps.jsonl')
reg = unreal.AssetRegistryHelpers.get_asset_registry()
lib = unreal.EditorAssetLibrary

todo = []
for kit in ('KiteDemo', 'ParagonProps', 'SoulCave'):
    for d in reg.get_assets_by_path('/Game/' + kit, recursive=True):
        if str(d.asset_class_path.asset_name) not in ('Texture2D', 'TextureCube'):
            continue
        dims = str(d.get_tag_value('Dimensions') or '')
        fmt = str(d.get_tag_value('Format') or '')
        try:
            big = max(int(x) for x in dims.split('x')[:2])
        except ValueError:
            continue
        limit = 1024 if 'FloatRGBA' in fmt else 2048
        if big > limit:
            todo.append((str(d.package_name), dims, fmt, limit))

changed = skipped = 0
with unreal.ScopedSlowTask(len(todo), 'Capping kit textures') as task:
    task.make_dialog(True)
    for i, (pkg, dims, fmt, limit) in enumerate(todo):
        if task.should_cancel():
            break
        task.enter_progress_frame(1, pkg.split('/')[-1])
        tex = unreal.load_asset(pkg)
        before = tex.get_editor_property('max_texture_size')
        if before and before <= limit:
            skipped += 1
        else:
            tex.set_editor_property('max_texture_size', limit)
            lib.save_loaded_asset(tex, False)
            with open(LOG, 'a') as f:
                f.write(json.dumps([pkg, dims, fmt, before, limit]) + '\n')
            changed += 1
        tex = None
        if (i + 1) % BATCH == 0:
            unreal.SystemLibrary.collect_garbage()

unreal.SystemLibrary.collect_garbage()
json.dump({'candidates': len(todo), 'changed_this_run': changed, 'already_capped': skipped},
          open(os.path.join(SAVED, 'kit_texture_caps.json'), 'w'), indent=0)
unreal.log('[KitCaps] candidates {} changed {} already {}'.format(len(todo), changed, skipped))
