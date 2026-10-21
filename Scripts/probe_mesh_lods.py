"""LOD table of every static mesh under a folder: py ".../probe_mesh_lods.py" /Game/Path -> Saved/mesh_lods.json
Per LOD: vertices and the screen size it switches at; Nanite flag; first material's blend mode."""
import json, os, sys
import unreal
root = sys.argv[1]
sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
reg = unreal.AssetRegistryHelpers.get_asset_registry()
out = []
for d in reg.get_assets_by_path(root, recursive=True):
    if str(d.asset_class_path.asset_name) != 'StaticMesh':
        continue
    m = unreal.load_asset(str(d.package_name))
    n = sm.get_lod_count(m)
    sizes = sm.get_lod_screen_sizes(m)
    out.append({'mesh': m.get_name(), 'nanite': m.get_editor_property('nanite_settings').enabled,
                'lods': [{'verts': sm.get_number_verts(m, i), 'screen': round(sizes[i], 3) if i < len(sizes) else None} for i in range(n)],
                'materials': [s.material_interface.get_name() if s.material_interface else None for s in m.static_materials]})
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'mesh_lods.json'), 'w'), indent=1)
