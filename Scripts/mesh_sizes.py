"""Read-only, registry: approximate size (cm, from the ApproxSize tag) of every StaticMesh / SkeletalMesh under /Game.
py ".../mesh_sizes.py" -> Saved/mesh_sizes.json {package: [x, y, z]}"""
import json, os
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
out = {}
for d in reg.get_assets_by_path('/Game', recursive=True):
    if str(d.asset_class_path.asset_name) not in ('StaticMesh', 'SkeletalMesh'):
        continue
    v = str(d.get_tag_value('ApproxSize') or '')
    try:
        out[str(d.package_name)] = [float(x) for x in v.split('x')[:3]]
    except ValueError:
        out[str(d.package_name)] = None
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'mesh_sizes.json'), 'w'))
