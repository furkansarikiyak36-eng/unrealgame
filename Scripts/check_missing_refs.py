"""Read-only: references to packages that no longer exist, from the raw-log materials and from Main / PerfTest.
Saved/check_missing_refs.json"""
import collections, json, os
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
exists = lambda p: unreal.EditorAssetLibrary.does_asset_exist(p)
out = {'rawlog': {}, 'maps': {}}
for m in ('/Game/UnrealExport/Materials/M_RawLog', '/Game/UnrealExport/Materials/MI_RawLog_Bark', '/Game/UnrealExport/Materials/MI_RawLog_Cut',
          '/Game/UnrealExport/Materials/MI_RawLog_Hewn', '/Game/UnrealExport/Materials/MI_RawLog_Peeled', '/Game/UnrealExport/Materials/MI_RawLog_PeeledCut'):
    out['rawlog'][m] = {str(d): exists(str(d)) for d in (reg.get_dependencies(m, opts) or []) if str(d).startswith('/Game/')}
for mp in ('/Game/Maps/Main', '/Game/Maps/PerfTest'):
    deps = [str(d) for d in (reg.get_dependencies(mp, opts) or []) if str(d).startswith('/Game/')]
    missing = [d for d in deps if not exists(d)]
    out['maps'][mp] = {'direct_deps': len(deps), 'missing': len(missing),
                       'missing_by_folder': dict(collections.Counter('/'.join(d.split('/')[:3]) for d in missing))}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'check_missing_refs.json'), 'w'), indent=1)
