"""For every StaticMesh and SkeletalMesh under /Game: the materials and textures it pulls in (recursive dependencies,
through material instances, parents and functions; nothing is loaded). Saved/mesh_dependencies.json:
{mesh package: {"materials": [...], "textures": [...]}}"""
import json
import os

import unreal

reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
cls_of = {str(d.package_name): str(d.asset_class_path.asset_name) for d in reg.get_assets_by_path('/Game', recursive=True)}
memo = {}


def textures_of(pkg, depth=0):
    """Textures reachable from a material-ish package."""
    if pkg in memo:
        return memo[pkg]
    memo[pkg] = set()
    out = set()
    for d in reg.get_dependencies(pkg, opts) or []:
        d = str(d)
        c = cls_of.get(d)
        if c in ('Texture2D', 'TextureCube', 'VirtualTexture2D', 'Texture2DArray'):
            out.add(d)
        elif c in ('Material', 'MaterialInstanceConstant', 'MaterialFunction', 'MaterialFunctionMaterialLayer',
                   'MaterialFunctionMaterialLayerBlend', 'MaterialParameterCollection') and depth < 12:
            out |= textures_of(d, depth + 1)
    memo[pkg] = out
    return out


result = {}
for pkg, c in cls_of.items():
    if c not in ('StaticMesh', 'SkeletalMesh'):
        continue
    mats, tex = set(), set()
    for d in reg.get_dependencies(pkg, opts) or []:
        d = str(d)
        dc = cls_of.get(d)
        if dc in ('Material', 'MaterialInstanceConstant'):
            mats.add(d)
            tex |= textures_of(d)
        elif dc in ('Texture2D', 'TextureCube'):
            tex.add(d)
    result[pkg] = {'materials': sorted(mats), 'textures': sorted(tex)}
json.dump(result, open(os.path.join(unreal.Paths.project_saved_dir(), 'mesh_dependencies.json'), 'w'))
unreal.log('[MeshDeps] {} meshes'.format(len(result)))
