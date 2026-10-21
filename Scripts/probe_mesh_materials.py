"""Read-only: for each given static mesh, its material slots -> material, parent chain, blend mode, two-sided, Nanite,
texture params. py ".../probe_mesh_materials.py" /Game/a,/Game/b -> Saved/mesh_materials.json"""
import json, os, sys
import unreal
out = {}
for p in sys.argv[1].split(','):
    m = unreal.load_asset(p)
    if not m:
        out[p] = 'missing'; continue
    rows = []
    for i, s in enumerate(m.static_materials):
        mi = s.material_interface
        row = {'slot': str(s.material_slot_name), 'mat': mi.get_path_name() if mi else None}
        if mi:
            chain, cur = [], mi
            while isinstance(cur, unreal.MaterialInstance):
                cur = cur.get_editor_property('parent'); chain.append(cur.get_path_name() if cur else None)
            row['parents'] = chain
            base = mi.get_base_material()
            row['blend'] = str(base.get_editor_property('blend_mode')); row['two_sided'] = base.get_editor_property('two_sided')
            row['used_nanite'] = base.get_editor_property('used_with_nanite') if hasattr(base, 'get_editor_property') else None
            try:
                row['tex'] = {str(n): (unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi, n).get_path_name()
                              if unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi, n) else None)
                              for n in unreal.MaterialEditingLibrary.get_texture_parameter_names(mi)} if isinstance(mi, unreal.MaterialInstance) else None
                row['scalars'] = {str(n): unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mi, n)
                                  for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(mi)} if isinstance(mi, unreal.MaterialInstance) else None
            except Exception as e:
                row['err'] = str(e)[:120]
            row['base_textures'] = [t.get_path_name() for t in unreal.MaterialEditingLibrary.get_used_textures(base)]
        rows.append(row)
    out[p] = {'nanite': m.get_editor_property('nanite_settings').enabled, 'slots': rows}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'mesh_materials.json'), 'w'), indent=1)
