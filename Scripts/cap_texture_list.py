"""Sets Max Texture Size on the listed textures (only lowers it). py ".../cap_texture_list.py" <size> /Game/t1,/Game/t2
Undo log ([package, previous, new]) appended to Saved/texture_list_caps.jsonl; summary Saved/cap_texture_list.json"""
import json, os, sys
import unreal
size = int(sys.argv[1]); LOG = os.path.join(unreal.Paths.project_saved_dir(), 'texture_list_caps.jsonl'); done = []
for p in sys.argv[2].split(','):
    t = unreal.load_asset(p)
    if not t:
        done.append([p, 'missing']); continue
    prev = t.get_editor_property('max_texture_size')
    if prev and prev <= size:
        done.append([p, prev, 'kept']); continue
    t.set_editor_property('max_texture_size', size)
    unreal.EditorAssetLibrary.save_loaded_asset(t, False)
    row = [p, prev, size]; done.append(row)
    with open(LOG, 'a') as f: f.write(json.dumps(row) + '\n')
json.dump(done, open(os.path.join(unreal.Paths.project_saved_dir(), 'cap_texture_list.json'), 'w'), indent=1)
