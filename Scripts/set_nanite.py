"""Nanite on/off for named static meshes: py ".../set_nanite.py" on|off /Game/Path/Mesh1,/Game/Path/Mesh2  -> Saved/set_nanite.json"""
import json, os, sys
import unreal
on = sys.argv[1] == 'on'
sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
out = []
for p in sys.argv[2].split(','):
    m = unreal.load_asset(p)
    s = m.get_editor_property('nanite_settings')
    if s.enabled != on:
        s.enabled = on
        sm.set_nanite_settings(m, s, True)
        unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    out.append([p, on])
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'set_nanite.json'), 'w'), indent=0)
