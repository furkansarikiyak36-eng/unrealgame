"""PN_GrassLibrary meshes: Nanite on/off, LODs, LOD0 vertices/triangles, materials: Saved/pn_grass.json"""
import json, os
import unreal
sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
reg = unreal.AssetRegistryHelpers.get_asset_registry()
out = []
for d in reg.get_assets_by_path('/Game/PN_GrassLibrary/Meshes', recursive=True):
    if str(d.asset_class_path.asset_name) != 'StaticMesh':
        continue
    m = unreal.load_asset(str(d.package_name))
    mats = [str(s.material_interface.get_path_name()) if s.material_interface else None for s in m.static_materials]
    blend = []
    for p in mats:
        mi = unreal.load_asset(p.split('.')[0]) if p else None
        base = mi.get_base_material() if mi and hasattr(mi, 'get_base_material') else mi
        blend.append((base.get_name(), str(base.get_editor_property('blend_mode')), str(base.get_editor_property('shading_model'))) if base else None)
    out.append({'mesh': m.get_name(), 'nanite': m.get_editor_property('nanite_settings').enabled, 'lods': sm.get_lod_count(m),
                'verts_lod0': sm.get_number_verts(m, 0),
                'tris_lod0': m.get_num_triangles(0) if hasattr(m, 'get_num_triangles') else None,
                'materials': blend})
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pn_grass.json'), 'w'), indent=1)
