"""Referencers (hard + soft, /Game only) of every package in Saved/heavy_packages.json: Saved/heavy_referencers.json"""
import json, os
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
pk = json.load(open(os.path.join(unreal.Paths.project_saved_dir(), 'heavy_packages.json')))
out = {p: sorted(str(r) for r in (reg.get_referencers(p, opts) or []) if str(r).startswith('/Game/')) for p in pk}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'heavy_referencers.json'), 'w'), indent=0)
