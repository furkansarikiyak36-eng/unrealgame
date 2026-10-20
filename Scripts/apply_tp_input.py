"""Adds the third-person template's legacy input names to the running editor's input settings (the same lines are in
Config/DefaultInput.ini, so this is only needed until the next restart). Report: Saved/tp_input.json"""
import json, os
import unreal
s = unreal.InputSettings.get_input_settings()
def key(n):
    k = unreal.Key(); k.set_editor_property('key_name', n); return k
have = {(str(m.axis_name), str(m.key.get_editor_property('key_name'))) for m in s.get_editor_property('axis_mappings')}
added = []
for axis, k, sc in [('MoveForward', 'W', 1.0), ('MoveForward', 'S', -1.0), ('MoveForward', 'Gamepad_LeftY', 1.0), ('MoveRight', 'D', 1.0),
                    ('MoveRight', 'A', -1.0), ('MoveRight', 'Gamepad_LeftX', 1.0), ('Turn', 'MouseX', 1.0), ('LookUp', 'MouseY', -1.0),
                    ('TurnRate', 'Gamepad_RightX', 1.0), ('LookUpRate', 'Gamepad_RightY', -1.0)]:
    if (axis, k) not in have:
        s.add_axis_mapping(unreal.InputAxisKeyMapping(axis_name=axis, key=key(k), scale=sc), False); added.append(axis + ':' + k)
acts = {(str(m.action_name), str(m.key.get_editor_property('key_name'))) for m in s.get_editor_property('action_mappings')}
for k in ('SpaceBar', 'Gamepad_FaceButton_Bottom'):
    if ('Jump', k) not in acts:
        s.add_action_mapping(unreal.InputActionKeyMapping(action_name='Jump', key=key(k)), False); added.append('Jump:' + k)
s.force_rebuild_keymaps()
json.dump({'added': added, 'axis_total': len(s.get_editor_property('axis_mappings'))}, open(os.path.join(unreal.Paths.project_saved_dir(), 'tp_input.json'), 'w'))
