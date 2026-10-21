"""Everything a placeable mesh drags in when it is used: the full hard-dependency tree of every StaticMesh and SkeletalMesh
under /Game (part meshes of Nanite assemblies, skeleton, physics asset, materials, material functions, textures...).
Nothing is loaded. Saved/bundle_dependencies.json: {mesh package: [dependency packages]} (the mesh itself excluded).
Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/bundle_dependencies.py"
"""
import json
import os

import unreal

reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
cls_of = {str(d.package_name): str(d.asset_class_path.asset_name) for d in reg.get_assets_by_path('/Game', recursive=True)}
direct = {}


def deps(p):
    if p not in direct:
        direct[p] = [str(d) for d in (reg.get_dependencies(p, opts) or []) if str(d) in cls_of]
    return direct[p]


# Generator data of Megaplant trees: kept with the tree in the editor, not loaded by the game. Listed, not followed.
EDITOR_ONLY = ('ProceduralVegetation', 'ProceduralVegetationGrowthDataAsset', 'ProceduralVegetationGrowerPreset')
out, editor_only = {}, {}
for pkg, c in cls_of.items():
    if c not in ('StaticMesh', 'SkeletalMesh'):
        continue
    seen, todo, eo = set(), [pkg], set()
    while todo:
        for d in deps(todo.pop()):
            if cls_of.get(d) in EDITOR_ONLY:
                eo.add(d)
                continue
            if d not in seen and d != pkg and cls_of.get(d) not in ('World', 'MapBuildDataRegistry'):
                seen.add(d)
                todo.append(d)
    out[pkg] = sorted(seen)
    if eo:
        editor_only[pkg] = sorted(eo)
json.dump({'runtime': out, 'editor_only': editor_only}, open(os.path.join(unreal.Paths.project_saved_dir(), 'bundle_dependencies.json'), 'w'))
unreal.log('[Bundles] {} meshes'.format(len(out)))
