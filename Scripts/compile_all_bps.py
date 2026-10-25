"""Compiles every Blueprint under a folder and saves the ones that compile: py ".../compile_all_bps.py" /Game/Folder
Report: Saved/compile_all_bps.json (status per blueprint)."""
import json, os, sys
import unreal
root = sys.argv[1] if len(sys.argv) > 1 else '/Game/PN_GrassLibrary'
reg = unreal.AssetRegistryHelpers.get_asset_registry()
out = {}
for d in reg.get_assets_by_path(root, recursive=True):
    if str(d.asset_class_path.asset_name) not in ('Blueprint', 'AnimBlueprint', 'WidgetBlueprint'):
        continue
    bp = unreal.load_asset(str(d.package_name))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    st = str(bp.get_editor_property('status')).split('.')[-1].split(':')[0]
    out[str(d.package_name)] = st
    if st in ('BS_UP_TO_DATE', 'BS_UP_TO_DATE_WITH_WARNINGS'):
        unreal.EditorAssetLibrary.save_asset(str(d.package_name), False)
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'compile_all_bps.json'), 'w'), indent=1)
