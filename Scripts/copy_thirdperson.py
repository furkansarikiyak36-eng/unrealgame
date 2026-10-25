"""Copies the PN_GrassLibrary third-person template into /Game/PerfTest/ThirdPerson (BP_TP_Character, BP_TP_GameMode),
points the copied game mode at the copied character, and makes it PerfTest's GameMode Override.
py ".../copy_thirdperson.py"   Report: Saved/copy_thirdperson.json"""
import json, os
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
SRC = '/Game/PN_GrassLibrary/UE4ThirdPersonTemplate/ThirdPersonBP/Blueprints/'
DST = '/Game/PerfTest/ThirdPerson/'
lib = unreal.EditorAssetLibrary
for src, dst in (('ThirdPersonCharacter', 'BP_TP_Character'), ('ThirdPersonGameMode', 'BP_TP_GameMode')):
    if not lib.does_asset_exist(DST + dst):
        lib.duplicate_asset(SRC + src, DST + dst)
char_class = unreal.load_class(None, DST + 'BP_TP_Character.BP_TP_Character_C')
gm_bp = unreal.load_asset(DST + 'BP_TP_GameMode')
gm_class = unreal.load_class(None, DST + 'BP_TP_GameMode.BP_TP_GameMode_C')
unreal.get_default_object(gm_class).set_editor_property('default_pawn_class', char_class)
unreal.BlueprintEditorLibrary.compile_blueprint(gm_bp)
lib.save_asset(DST + 'BP_TP_GameMode', False)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode', gm_class)
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
graphs = [g.get_path_name() for g in unreal.BlueprintEditorLibrary.find_event_graph(unreal.load_asset(DST + 'BP_TP_Character')) and [unreal.BlueprintEditorLibrary.find_event_graph(unreal.load_asset(DST + 'BP_TP_Character'))] or []]
json.dump({'character': char_class.get_path_name(), 'game_mode': gm_class.get_path_name(),
           'pawn_of_game_mode': unreal.get_default_object(gm_class).get_editor_property('default_pawn_class').get_path_name(),
           'event_graph': graphs}, open(os.path.join(unreal.Paths.project_saved_dir(), 'copy_thirdperson.json'), 'w'), indent=1)
