"""Brings back the raw-log wood textures (wooden_fence_albedo / _normal) that were lost with the ProceduralNtr_vol2 kit.
The two .uasset files are first copied by hand from Downloads/MyProject into their old path; this script registers them,
moves them to /Game/UnrealExport/Textures (references follow), resaves the five raw-log materials, removes the
redirectors and the empty kit folder, and caps both textures at 1024 px.
py ".../restore_rawlog_textures.py" -> Saved/restore_rawlog_textures.json"""
import json, os
import unreal
lib = unreal.EditorAssetLibrary
reg = unreal.AssetRegistryHelpers.get_asset_registry()
OLD = '/Game/ProceduralNtr_vol2/pnp_v2_assets/Assets/wood_fence/Textures/'
NEW = '/Game/UnrealExport/Textures/'
reg.scan_paths_synchronous(['/Game/ProceduralNtr_vol2'], True)
rep = {'moved': [], 'saved': [], 'caps': []}
for n in ('wooden_fence_albedo', 'wooden_fence_normal'):
    if lib.does_asset_exist(OLD + n) and not lib.does_asset_exist(NEW + n):
        if lib.rename_asset(OLD + n, NEW + n):
            rep['moved'].append(n)
for m in ('M_RawLog', 'MI_RawLog_Cut', 'MI_RawLog_Hewn', 'MI_RawLog_Peeled', 'MI_RawLog_PeeledCut'):
    p = '/Game/UnrealExport/Materials/' + m
    a = unreal.load_asset(p)
    if a and lib.save_loaded_asset(a, False):
        rep['saved'].append(m)
for d in reg.get_assets_by_path('/Game/ProceduralNtr_vol2', recursive=True):
    if str(d.asset_class_path.asset_name) == 'ObjectRedirector':
        lib.delete_asset(str(d.package_name))
if not reg.get_assets_by_path('/Game/ProceduralNtr_vol2', recursive=True):
    lib.delete_directory('/Game/ProceduralNtr_vol2')
for n in ('wooden_fence_albedo', 'wooden_fence_normal'):
    t = unreal.load_asset(NEW + n)
    if t:
        rep['caps'].append([n, t.blueprint_get_size_x(), t.get_editor_property('max_texture_size')])
        t.set_editor_property('max_texture_size', 1024)
        lib.save_loaded_asset(t, False)
rep['folder_left'] = lib.does_directory_exist('/Game/ProceduralNtr_vol2')
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'restore_rawlog_textures.json'), 'w'), indent=1)
