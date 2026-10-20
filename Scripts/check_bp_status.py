"""Compiles and saves the PerfTest third-person blueprints, reports their status: Saved/bp_status.json"""
import json, os
import unreal
out = {}
for p in ('/Game/PerfTest/ThirdPerson/BP_TP_Character', '/Game/PerfTest/ThirdPerson/BP_TP_GameMode'):
    bp = unreal.load_asset(p)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    out[p] = str(bp.get_editor_property('status'))
    unreal.EditorAssetLibrary.save_asset(p, False)
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'bp_status.json'), 'w'), indent=1)
