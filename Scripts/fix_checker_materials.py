"""Gives a real material to environment meshes whose slots use the engine checker material (WorldGridMaterial) in
KiteDemo / ParagonProps / SoulCave. FX meshes are left alone (their particle systems set the material at runtime).
Sibling meshes' materials are used where the kit has one; slots with nothing to copy get the new Megascans surfaces.
py ".../fix_checker_materials.py"     Undo log (mesh, slot index, old, new): Saved/fix_checker_materials.jsonl
"""
import json, os
import unreal
MS = '/Game/Fab/Megascans/Surfaces/'
PD = '/Game/ParagonProps/Monolith/Dawn/Materials/'
FIX = {
    '/Game/SoulCave/Environment/Meshes/Building/SM_Cave_Pillar_NoFlag': {'*': '/Game/SoulCave/Environment/Materials/Building/M_Cave_Pillar'},
    '/Game/SoulCave/Environment/Meshes/Rocks/SM_Rocks_Flat_Top': {'*': MS + 'Mine_Rock_Wall_ueknfaclw/Low/ueknfaclw_tier_3/Materials/MI_ueknfaclw'},
    '/Game/SoulCave/Environment/Meshes/Nature/SM_Ground_Plane_Hump01': {'*': MS + 'Coal_Mine_Floor_ud0nfezew/Low/ud0nfezew_tier_3/Materials/MI_ud0nfezew'},
    '/Game/SoulCave/Environment/Meshes/Nature/SM_Ground_Plane_Warble': {'*': MS + 'Coal_Mine_Floor_ud0nfezew/Low/ud0nfezew_tier_3/Materials/MI_ud0nfezew'},
    '/Game/ParagonProps/Monolith/Dawn/Meshes/DawnSide_Pillar': {'*': PD + 'Monolith_DawnArchitecture_AllMetal'},
    '/Game/ParagonProps/Monolith/Dawn/Meshes/Dawn_TerminusCore_Top_X': {'*': PD + 'Monolith_DawnArchitecture_NoNormal'},
    '/Game/ParagonProps/Monolith/Dawn/Meshes/Dawn_Wall_Elaborate_Straight_A': {'Material #16387': PD + 'M_Dawnside_Walls_V3_Part1',
                                                                              '*': PD + 'M_Dawnside_Walls_V3_Part2'},
}
LOG = os.path.join(unreal.Paths.project_saved_dir(), 'fix_checker_materials.jsonl')
done = []
for mesh_path, rule in FIX.items():
    mesh = unreal.load_asset(mesh_path)
    if not mesh:
        continue
    changed = False
    for i, s in enumerate(mesh.static_materials):
        cur = s.material_interface
        if cur and cur.get_name() != 'WorldGridMaterial':
            continue
        new_path = rule.get(str(s.material_slot_name), rule['*'])
        new = unreal.load_asset(new_path)
        if not new:
            continue
        mesh.set_material(i, new)
        changed = True
        row = [mesh_path, i, cur.get_path_name() if cur else None, new_path]
        done.append(row)
        with open(LOG, 'a') as f:
            f.write(json.dumps(row) + '\n')
    if changed:
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
json.dump(done, open(os.path.join(unreal.Paths.project_saved_dir(), 'fix_checker_materials.json'), 'w'), indent=1)
