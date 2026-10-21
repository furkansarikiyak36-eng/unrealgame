"""Sets the raw-log wood textures explicitly: Base Color / Normal on MI_RawLog_Cut/Hewn/Peeled/PeeledCut, and the empty
texture parameter defaults in M_RawLog (the references were nulled while the textures were missing).
py ".../relink_rawlog_textures.py" -> Saved/relink_rawlog_textures.json"""
import json, os
import unreal
MEL = unreal.MaterialEditingLibrary; lib = unreal.EditorAssetLibrary
T = '/Game/UnrealExport/Textures/'
alb = unreal.load_asset(T + 'wooden_fence_albedo'); nrm = unreal.load_asset(T + 'wooden_fence_normal')
rep = {'mi': {}, 'parent_nodes': []}
base = unreal.load_asset('/Game/UnrealExport/Materials/M_RawLog')
for ex in unreal.ObjectIterator(unreal.MaterialExpressionTextureSampleParameter2D):
    if ex.get_outermost() == base.get_outermost() and ex.get_editor_property('texture') is None:
        name = str(ex.get_editor_property('parameter_name'))
        tex = nrm if 'Normal' in name and 'End' not in name else (alb if name == 'Base Color' else None)
        if tex:
            ex.set_editor_property('texture', tex); rep['parent_nodes'].append(name)
if rep['parent_nodes']:
    MEL.recompile_material(base); lib.save_loaded_asset(base, False)
for n in ('Cut', 'Hewn', 'Peeled', 'PeeledCut'):
    mi = unreal.load_asset('/Game/UnrealExport/Materials/MI_RawLog_' + n)
    a = MEL.set_material_instance_texture_parameter_value(mi, 'Base Color', alb)
    b = MEL.set_material_instance_texture_parameter_value(mi, 'Normal', nrm)
    MEL.update_material_instance(mi); lib.save_loaded_asset(mi, False)
    rep['mi'][n] = [a, b]
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'relink_rawlog_textures.json'), 'w'), indent=1)
