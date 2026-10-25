"""Read-only, registry: for each texture, the materials that use it and the static meshes using those materials.
py ".../texture_users.py" /Game/t1,/Game/t2 -> Saved/texture_users.json"""
import json, os, sys
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(True, True, False, False, False)
def cls(p):
    a = reg.get_assets_by_package_name(p); return str(a[0].asset_class_path.asset_name) if a else ''
out = {}
for t in sys.argv[1].split(','):
    mats, meshes, seen, todo = [], set(), set(), [t]
    while todo:
        cur = todo.pop()
        for r in reg.get_referencers(cur, opts) or []:
            r = str(r)
            if r in seen: continue
            seen.add(r); c = cls(r)
            if c in ('Material', 'MaterialInstanceConstant', 'MaterialFunction'):
                mats.append(r); todo.append(r)
            elif c == 'StaticMesh':
                meshes.add(r)
    out[t] = {'materials': mats, 'meshes': sorted(meshes)}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'texture_users.json'), 'w'), indent=1)
