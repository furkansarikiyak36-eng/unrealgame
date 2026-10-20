"""Heights (cm) of the Megascans shrub and plant meshes: Saved/shrub_heights.json"""
import json, os, unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
out = {}
for f in ['Raspberry_wf0oefeja', 'Elderberry_wfzobb2ia', 'European_Spindle_wk1ncbxja', 'Black_Locust_wdvhberja', 'Dead_Shrubs_vdknafgha',
          'Bigleaf_Hydrangea_vgztealha', 'Dragon_Tree_siumT', 'Alexandra_Palm_ukgkcilia', 'Pink_Cordyline_sfhhN', 'Greater_Burdock_xh5pdgiqx', 'Lady_Fern_wdvlditia']:
    hs = []
    for d in reg.get_assets_by_path('/Game/Fab/Megascans/Plants/' + f, recursive=True):
        if str(d.asset_class_path.asset_name) == 'StaticMesh':
            b = unreal.load_asset(str(d.package_name)).get_bounding_box()
            hs.append((str(d.asset_name), round(b.max.z - b.min.z), round(max(b.max.x - b.min.x, b.max.y - b.min.y))))
    out[f] = sorted(hs, key=lambda h: -h[1])
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'shrub_heights.json'), 'w'), indent=0)
