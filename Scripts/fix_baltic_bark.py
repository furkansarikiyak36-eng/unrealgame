"""Points MI_Baltic_Pine_01_Bark's four stale texture parameters at the pack's own bark textures and saves it.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/fix_baltic_bark.py"
The download kept references to textures on the pack author's machine (/Game/Developers/simonbarle/...) and in the
Aleppo Pine pack, which are not in this project; every load logged four LoadErrors. The material blends the
Lower/Upper bark (already the pack's own textures); these four only need to point at something that exists.
Writes Saved/fix_baltic_bark.json.
"""
import json
import os

import unreal

MI = '/Game/Megaplant_Library/Tree_Baltic_Pine/Materials/MI_Baltic_Pine_01_Bark'
TEX = '/Game/Megaplant_Library/Tree_Baltic_Pine/Textures/'
FIX = {
    'Base Color': 'T_Baltic_PineBark_02_C',
    'Normal': 'T_Baltic_PineBark_02_NAH',
    'Branch Base Color': 'T_Baltic_PineBark_01_C',
    'Branch Normal': 'T_Baltic_PineBark_01_NAH',
}
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'fix_baltic_bark.json')


def main():
    mi = unreal.load_asset(MI)
    lib = unreal.MaterialEditingLibrary
    report = {}
    for param, tex_name in FIX.items():
        tex = unreal.load_asset(TEX + tex_name)
        ok = lib.set_material_instance_texture_parameter_value(mi, param, tex)
        report[param] = [tex_name, bool(ok), str(lib.get_material_instance_texture_parameter_value(mi, param).get_name())]
    lib.update_material_instance(mi)
    report['saved'] = unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[FixBalticBark] ' + json.dumps(report))


main()
