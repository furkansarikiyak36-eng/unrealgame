"""Read-only: what the raw-log materials reference now, and which of those packages exist. Saved/check_rawlog_refs.json"""
import json, os
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
out = {}
for m in ('/Game/UnrealExport/Materials/M_RawLog', '/Game/UnrealExport/Materials/MI_RawLog_Bark', '/Game/UnrealExport/Materials/MI_RawLog_Cut',
          '/Game/UnrealExport/Materials/MI_RawLog_Hewn', '/Game/UnrealExport/Materials/MI_RawLog_Peeled', '/Game/UnrealExport/Materials/MI_RawLog_PeeledCut'):
    deps = [str(d) for d in (reg.get_dependencies(m, opts) or []) if str(d).startswith('/Game/')]
    out[m] = {d: unreal.EditorAssetLibrary.does_asset_exist(d) for d in deps}
redirs = [str(d.package_name) for d in reg.get_assets_by_path('/Game', recursive=True) if str(d.asset_class_path.asset_name) == 'ObjectRedirector']
out['_redirectors'] = redirs
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'check_rawlog_refs.json'), 'w'), indent=1)
