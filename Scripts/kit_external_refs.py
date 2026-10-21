"""Which assets outside the kit folders listed in Saved/kits_to_delete.json reference something inside them (hard + soft)?
Saved/kit_external_refs.json: {referencer: [kit packages it uses]}"""
import json, os
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
kits = json.load(open(os.path.join(unreal.Paths.project_saved_dir(), 'kits_to_delete.json')))
inside = lambda p: any(p.startswith(k + '/') for k in kits)
out = {}
for d in reg.get_assets_by_path('/Game', recursive=True):
    p = str(d.package_name)
    if inside(p):
        continue
    used = [str(x) for x in (reg.get_dependencies(p, opts) or []) if inside(str(x))]
    if used:
        out[p] = sorted(set(used))
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'kit_external_refs.json'), 'w'), indent=1)
