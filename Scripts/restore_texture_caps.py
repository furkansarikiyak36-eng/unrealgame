"""Puts back the Max Texture Size of textures that already had a smaller-or-equal cap before cleanup_heavy.py
(it set every texture to 2048). Reads Saved/cleanup_heavy.json. Report: Saved/restore_texture_caps.json"""
import json, os
import unreal
SAVED = unreal.Paths.project_saved_dir()
r = json.load(open(os.path.join(SAVED, 'cleanup_heavy.json')))
done = []
for c in r['capped']:
    if c['before'] and c['before'] <= 2048:
        tex = unreal.load_asset(c['texture'])
        tex.set_editor_property('max_texture_size', c['before'])
        unreal.EditorAssetLibrary.save_loaded_asset(tex, False)
        done.append([c['texture'], c['before']])
json.dump(done, open(os.path.join(SAVED, 'restore_texture_caps.json'), 'w'), indent=0)
