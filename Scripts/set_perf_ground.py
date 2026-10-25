"""Ground surface of /Game/Maps/PerfTest: py ".../set_perf_ground.py" <Megascans surface folder> [tiling]
Re-parents MI_PerfGround to that surface's material instance (the 3.5 km cube top is one UV tile, so tiling 1750 =
one repeat per 2 m). Report: Saved/perf_ground.json."""
import json, os, sys
import unreal
folder = sys.argv[1] if len(sys.argv) > 1 else 'Wild_Grass_sfknaeoa'
tiling = float(sys.argv[2]) if len(sys.argv) > 2 else 1750.0
reg = unreal.AssetRegistryHelpers.get_asset_registry()
mis = [str(d.package_name) for d in reg.get_assets_by_path('/Game/Fab/Megascans/Surfaces/' + folder, recursive=True)
       if str(d.asset_class_path.asset_name) == 'MaterialInstanceConstant']
mi = unreal.load_asset('/Game/PerfTest/MI_PerfGround')
unreal.MaterialEditingLibrary.set_material_instance_parent(mi, unreal.load_asset(mis[0]))
unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, 'Tiling', tiling)
unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
json.dump({'parent': mis[0], 'tiling': tiling}, open(os.path.join(unreal.Paths.project_saved_dir(), 'perf_ground.json'), 'w'))
