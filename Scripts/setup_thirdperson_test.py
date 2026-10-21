"""Third-person test character in /Game/Maps/PerfTest (the 50 x 50 m grass area).

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/setup_thirdperson_test.py"
- Uses the UE4 third-person template that ships with PN_GrassLibrary (ThirdPersonCharacter + ThirdPersonGameMode).
- That template reads legacy input names (MoveForward, MoveRight, Turn, LookUp, TurnRate, LookUpRate, Jump); they are
  added to the project input settings if missing (Config/DefaultInput.ini). The project's own character does not use them.
- PerfTest only: World Settings > GameMode Override = ThirdPersonGameMode; PlayerStart moved into the grass area.
Report: Saved/thirdperson_test.json
"""
import json
import os

import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))

BP = '/Game/PN_GrassLibrary/UE4ThirdPersonTemplate/ThirdPersonBP/Blueprints/'
report = {}

# Template blueprints compile?
for name in ('ThirdPersonCharacter', 'ThirdPersonGameMode'):
    bp = unreal.load_asset(BP + name)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    report[name] = str(bp.get_editor_property('status')) if bp else 'missing'
gm_class = unreal.load_class(None, BP + 'ThirdPersonGameMode.ThirdPersonGameMode_C')
pawn = unreal.get_default_object(gm_class).get_editor_property('default_pawn_class')
report['game_mode_pawn'] = pawn.get_name() if pawn else None

# Legacy input names the template binds to.
settings = unreal.InputSettings.get_input_settings()
have_axes = {str(m.axis_name) for m in settings.get_editor_property('axis_mappings')}
have_actions = {str(m.action_name) for m in settings.get_editor_property('action_mappings')}
axes = [('MoveForward', 'W', 1.0), ('MoveForward', 'S', -1.0), ('MoveRight', 'D', 1.0), ('MoveRight', 'A', -1.0),
        ('Turn', 'MouseX', 1.0), ('LookUp', 'MouseY', -1.0),
        ('MoveForward', 'Gamepad_LeftY', 1.0), ('MoveRight', 'Gamepad_LeftX', 1.0),
        ('TurnRate', 'Gamepad_RightX', 1.0), ('LookUpRate', 'Gamepad_RightY', -1.0)]
def key(name):
    k = unreal.Key()
    k.set_editor_property('key_name', name)
    return k


added = []
for axis, key_name, scale in axes:
    if axis in have_axes:
        continue
    settings.add_axis_mapping(unreal.InputAxisKeyMapping(axis_name=axis, key=key(key_name), scale=scale), False)
    added.append('{}:{}'.format(axis, key_name))
if 'Jump' not in have_actions:
    for key_name in ('SpaceBar', 'Gamepad_FaceButton_Bottom'):
        settings.add_action_mapping(unreal.InputActionKeyMapping(action_name='Jump', key=key(key_name)), False)
        added.append('Jump:' + key_name)
settings.force_rebuild_keymaps()
settings.save_key_mappings()
report['input_added'] = added

# PerfTest: game mode override and player start inside the grass area.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
ws = world.get_world_settings()
ws.set_editor_property('default_game_mode', gm_class)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
starts = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlayerStart)]
if not starts:
    starts = [actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 0))]
starts[0].set_actor_location(unreal.Vector(-1800.0, -1800.0, 100.0), False, False)
starts[0].set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=45.0), False)
report['player_start'] = [-1800, -1800, 100]
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
json.dump(report, open(os.path.join(unreal.Paths.project_saved_dir(), 'thirdperson_test.json'), 'w'), indent=1)
unreal.log('[ThirdPersonTest] ' + json.dumps(report))
