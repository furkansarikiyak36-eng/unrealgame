"""Moves what our own assets use out of the PN (Project Nature) kits before those kits are removed.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/relocate_pn_dependencies.py"
- PN_GrassLibrary third-person Mannequin (mesh, skeleton, animations, AnimBP) and PN_Bending_Component
  -> /Game/PerfTest/ThirdPerson/ (used by BP_TP_Character)
- PN_OpenWorldFoliage Mannequin (used by Main) -> /Game/Characters/Mannequin_OWF
References are moved with the assets (rename = move + redirectors); the redirectors are then fixed up so nothing keeps
pointing into the PN folders. Report: Saved/relocate_pn.json
"""
import json
import os

import unreal

lib = unreal.EditorAssetLibrary
MOVES = [
    ('/Game/PN_GrassLibrary/UE4ThirdPersonTemplate/Mannequin', '/Game/PerfTest/ThirdPerson/Mannequin'),
    ('/Game/PN_GrassLibrary/Blueprints/PN_Bending_Component', '/Game/PerfTest/ThirdPerson/PN_Bending_Component'),
    ('/Game/PN_OpenWorldFoliage/UE4_ThirdPerson/Mannequin', '/Game/Characters/Mannequin_OWF'),
]
report = {'moved': [], 'failed': [], 'redirectors_fixed': 0}
for src, dst in MOVES:
    try:
        if lib.does_directory_exist(src):
            ok = lib.rename_directory(src, dst)
        else:
            ok = lib.rename_asset(src, dst)
        (report['moved'] if ok else report['failed']).append([src, dst])
    except Exception as e:
        report['failed'].append([src, dst, str(e)[:150]])

reg = unreal.AssetRegistryHelpers.get_asset_registry()
redirs = []
for root in ('/Game/PN_GrassLibrary', '/Game/PN_OpenWorldFoliage'):
    for d in reg.get_assets_by_path(root, recursive=True):
        if str(d.asset_class_path.asset_name) == 'ObjectRedirector':
            obj = unreal.load_asset(str(d.package_name))
            if obj:
                redirs.append(obj)
if redirs:
    unreal.AssetToolsHelpers.get_asset_tools().fixup_referencers(redirs)
report['redirectors_fixed'] = len(redirs)
for bp in ('/Game/PerfTest/ThirdPerson/BP_TP_Character', '/Game/PerfTest/ThirdPerson/BP_TP_GameMode'):
    b = unreal.load_asset(bp)
    unreal.BlueprintEditorLibrary.compile_blueprint(b)
    report[bp.split('/')[-1]] = str(b.get_editor_property('status'))
    lib.save_asset(bp, False)
json.dump(report, open(os.path.join(unreal.Paths.project_saved_dir(), 'relocate_pn.json'), 'w'), indent=1)
unreal.log('[RelocatePN] ' + json.dumps({k: (len(v) if isinstance(v, list) else v) for k, v in report.items()}))
