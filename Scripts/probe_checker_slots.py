"""Read-only: for each given mesh, its slots (name -> material) and sibling meshes in the same folder with their slots.
py ".../probe_checker_slots.py" /Game/a,/Game/b -> Saved/checker_slots.json"""
import json, os, sys
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
def slots(m):
    return [[str(s.material_slot_name), s.material_interface.get_path_name() if s.material_interface else None] for s in m.static_materials]
out = {}
for p in sys.argv[1].split(','):
    m = unreal.load_asset(p)
    folder = p.rsplit('/', 1)[0]
    stem = ''.join(ch for ch in p.split('/')[-1] if not ch.isdigit()).rstrip('_').lower()[:12]
    sib = {}
    for d in reg.get_assets_by_path(folder, recursive=False):
        n = str(d.asset_name)
        if str(d.asset_class_path.asset_name) == 'StaticMesh' and str(d.package_name) != p and n.lower().startswith(stem[:8]):
            sm = unreal.load_asset(str(d.package_name)); sib[n] = slots(sm)
    out[p] = {'slots': slots(m), 'siblings': dict(list(sib.items())[:6])}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'checker_slots.json'), 'w'), indent=1)
