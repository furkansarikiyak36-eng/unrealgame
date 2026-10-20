"""Read-only: materials / material instances in the given kits whose dependencies (textures, parents, functions) do not
exist, and static meshes with no material. py ".../check_missing_material_deps.py" Kit1,Kit2 -> Saved/missing_material_deps.json"""
import collections, json, os, sys
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
lib = unreal.EditorAssetLibrary
out = {'materials_with_missing': {}, 'meshes_without_material': []}
for kit in sys.argv[1].split(','):
    for d in reg.get_assets_by_path('/Game/' + kit, recursive=True):
        c = str(d.asset_class_path.asset_name); p = str(d.package_name)
        if c in ('Material', 'MaterialInstanceConstant', 'MaterialFunction'):
            miss = [str(x) for x in (reg.get_dependencies(p, opts) or []) if str(x).startswith('/Game/') and not lib.does_asset_exist(str(x))]
            if miss:
                out['materials_with_missing'][p] = miss
        elif c == 'StaticMesh':
            deps = [str(x) for x in (reg.get_dependencies(p, opts) or [])]
            if not any('Material' in (reg.get_asset_by_object_path(x + '.' + x.split('/')[-1]).asset_class_path.asset_name.__str__() or '') for x in deps if x.startswith('/')):
                out['meshes_without_material'].append(p)
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'missing_material_deps.json'), 'w'), indent=1)
