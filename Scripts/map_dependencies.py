"""All /Game packages each map uses (hard + soft dependencies, recursive): Saved/map_dependencies.json"""
import json, os
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
out = {}
for m in ('/Game/Maps/Main', '/Game/Maps/PerfTest'):
    seen, todo = set(), [m]
    while todo:
        p = todo.pop()
        for d in reg.get_dependencies(p, opts) or []:
            d = str(d)
            if d.startswith('/Game/') and d not in seen:
                seen.add(d); todo.append(d)
    out[m] = sorted(seen)
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'map_dependencies.json'), 'w'), indent=0)
