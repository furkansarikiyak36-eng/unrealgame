"""Positions and bounds of named actors in the open level: py ".../probe_showcase_actor.py" Name1,Name2   -> Saved/showcase_actors.json"""
import json, os, sys
import unreal
names = sys.argv[1].split(',')
out = {}
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() in names:
        o, e = a.get_actor_bounds(False)
        out[a.get_actor_label()] = {'c': [round(o.x), round(o.y), round(o.z)], 'e': [round(e.x), round(e.y), round(e.z)]}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'showcase_actors.json'), 'w'), indent=1)
