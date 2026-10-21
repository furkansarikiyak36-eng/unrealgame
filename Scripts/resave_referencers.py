"""Loads and re-saves assets that still reference moved assets through redirectors, so they store the new paths:
BP_TP_Character / BP_TP_GameMode (compiled) and the Main map; then reopens PerfTest. Report: Saved/resave_referencers.json"""
import json, os
import unreal
lib = unreal.EditorAssetLibrary
out = {}
for bp in ('/Game/PerfTest/ThirdPerson/BP_TP_Character', '/Game/PerfTest/ThirdPerson/BP_TP_GameMode'):
    b = unreal.load_asset(bp)
    unreal.BlueprintEditorLibrary.compile_blueprint(b)
    out[bp] = str(b.get_editor_property('status'))
    b.modify()
    lib.save_loaded_asset(b, False)
unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/Main')
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
w.modify()
out['main_saved'] = unreal.EditorLoadingAndSavingUtils.save_map(w, '/Game/Maps/Main')
unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/PerfTest')
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'resave_referencers.json'), 'w'), indent=1)
