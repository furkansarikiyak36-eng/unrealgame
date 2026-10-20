"""Read-only: collision (simple shapes / convex hulls / complex-as-simple) and size of the Megascans rock and stone meshes
that a stone chain could use. py ".../probe_stone_meshes.py" -> Saved/stone_meshes.json"""
import json, os
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
FOLDERS = ['Mossy_Forest_Boulder_wfstcdnaw', 'Beach_Boulder_ukopeess', 'Beach_Boulder_ukopeijs', 'Nordic_Beach_Rocks_ulznddxva', 'Rock_shopk',
           'Sandstone_Boulder_vmjjfdm', 'Nordic_Beach_Rock_vbumba2fa', 'Beach_Boulder_wenobdn', 'Beach_Rock_wd3efb0',
           'Small_Stones_Pack_vcljbb1iw', 'Small_Stones_Pack_vdjkbfmiw', 'Mossy_Stones_Pack_tliiadmva', 'Military_Trenches_Scatter_Rock_S_10_ydxsabu',
           'Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx', 'Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx', 'Castle_Wall_Stone_sdAta']
out = {}
for f in FOLDERS:
    for d in reg.get_assets_by_path('/Game/Fab/Megascans/3D/' + f, recursive=True):
        if str(d.asset_class_path.asset_name) != 'StaticMesh':
            continue
        p = str(d.package_name); m = unreal.load_asset(p)
        b = m.get_bounding_box()
        out[p] = {'folder': f, 'size': [round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z)],
                  'simple': sm.get_simple_collision_count(m), 'convex': sm.get_convex_collision_count(m),
                  'complex_as_simple': str(m.get_editor_property('body_setup').get_editor_property('collision_trace_flag')) if m.get_editor_property('body_setup') else None,
                  'nanite': m.get_editor_property('nanite_settings').enabled}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'stone_meshes.json'), 'w'), indent=1)
