"""Read-only: height, width, triangles, LODs and Nanite of the PN_coniferBushes_I trees: Saved/conifers.json"""
import json, os
import unreal
sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
reg = unreal.AssetRegistryHelpers.get_asset_registry()
out = []
for d in reg.get_assets_by_path('/Game/PN_coniferBushes_I/Meshes', recursive=True):
    if str(d.asset_class_path.asset_name) != 'StaticMesh':
        continue
    m = unreal.load_asset(str(d.package_name))
    b = m.get_bounding_box()
    out.append({'mesh': m.get_name(), 'h_m': round((b.max.z - b.min.z) / 100, 2), 'w_m': round(max(b.max.x - b.min.x, b.max.y - b.min.y) / 100, 2),
                'tris': [m.get_num_triangles(i) for i in range(sm.get_lod_count(m))], 'nanite': m.get_editor_property('nanite_settings').enabled})
json.dump(sorted(out, key=lambda x: x['mesh']), open(os.path.join(unreal.Paths.project_saved_dir(), 'conifers.json'), 'w'), indent=0)
