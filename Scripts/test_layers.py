"""Shows/hides the test layers of the grass area in the editor (temporary, not saved):
py ".../test_layers.py" grass=1 trees=1 shrubs=1      Report: Saved/test_layers.json"""
import json, os, sys
import unreal
want = {k: v == '1' for k, v in (a.split('=') for a in sys.argv[1:])}
n = {'grass': 0, 'trees': 0, 'shrubs': 0}
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    tags = [str(t) for t in a.tags]
    layer = 'grass' if a.get_actor_label() == 'GrassScatterVolume' else 'trees' if 'ShrublandTrees' in tags else 'shrubs' if 'AreaShrubs' in tags else None
    if layer and layer in want:
        a.set_is_temporarily_hidden_in_editor(not want[layer]); n[layer] += 1
json.dump({'shown': want, 'actors': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'test_layers.json'), 'w'))
