"""Temporarily hides/shows GrassScatterVolume in the editor (measurement only): py ".../pcg_grass_hide.py" hide|show"""
import json, os, sys
import unreal
hide = (sys.argv[1] if len(sys.argv) > 1 else 'hide') == 'hide'
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == 'GrassScatterVolume':
        a.set_is_temporarily_hidden_in_editor(hide); n += 1
json.dump({'hidden': hide, 'actors': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'pcg_grass_hide.json'), 'w'))
