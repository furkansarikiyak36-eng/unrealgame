"""Read-only, registry only: static meshes in the given kits that reference no material asset (they render with the
engine default grey/checker material), and materials that reference only engine textures (look untextured).
py ".../check_mesh_materials.py" Kit1,Kit2 -> Saved/mesh_material_check.json"""
import json, os, sys
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
MAT = {'Material', 'MaterialInstanceConstant'}
cls_cache = {}
def cls(pkg):
    if pkg not in cls_cache:
        a = reg.get_assets_by_package_name(pkg)
        cls_cache[pkg] = str(a[0].asset_class_path.asset_name) if a else ''
    return cls_cache[pkg]
def textures_of(pkg, depth=0):
    tex = set()
    for d in reg.get_dependencies(pkg, opts) or []:
        d = str(d); c = cls(d)
        if c.startswith('Texture'):
            tex.add(d)
        elif c in MAT and depth < 4:
            tex |= textures_of(d, depth + 1)
    return tex
out = {'no_material': [], 'untextured_material': {}, 'missing_material': {}}
for kit in sys.argv[1].split(','):
    for d in reg.get_assets_by_path('/Game/' + kit, recursive=True):
        if str(d.asset_class_path.asset_name) != 'StaticMesh':
            continue
        p = str(d.package_name)
        deps = [str(x) for x in reg.get_dependencies(p, opts) or []]
        gone = [x for x in deps if x.startswith('/Game/') and not cls(x)]
        if gone:
            out['missing_material'][p] = gone
        mats = [x for x in deps if cls(x) in MAT]
        if not mats:
            out['no_material'].append(p); continue
        for m in mats:
            if not any(t.startswith('/Game/') for t in textures_of(m)):
                out['untextured_material'].setdefault(m, []).append(p)
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'mesh_material_check.json'), 'w'), indent=1)
