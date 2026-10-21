"""Creates /Game/UnrealExport/SM_RawLog_75: a copy of SM_RawLog_150 built at half length (Build Scale X 0.5), with its
simple collision rebuilt as one convex hull of the new shape. Skips if it already exists unless "force" is given.
py ".../make_rawlog_75.py" [force] -> Saved/make_rawlog_75.json"""
import json, os, sys
import unreal
lib = unreal.EditorAssetLibrary
SRC, DST = '/Game/UnrealExport/SM_RawLog_150', '/Game/UnrealExport/SM_RawLog_75'
force = len(sys.argv) > 1 and sys.argv[1] == 'force'
rep = {}
if lib.does_asset_exist(DST) and force:
    lib.delete_asset(DST)
if not lib.does_asset_exist(DST):
    lib.duplicate_asset(SRC, DST)
mesh = unreal.load_asset(DST)
sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
settings = sm.get_lod_build_settings(mesh, 0)
settings.set_editor_property('build_scale3d', unreal.Vector(0.5, 1.0, 1.0))
sm.set_lod_build_settings(mesh, 0, settings)
sm.remove_collisions(mesh)
sm.set_convex_decomposition_collisions(mesh, 1, 16, 100000)
lib.save_loaded_asset(mesh, False)
b = mesh.get_bounding_box()
rep = {'size': [round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z)], 'convex': sm.get_convex_collision_count(mesh),
       'materials': [str(m.material_slot_name) for m in mesh.static_materials]}
src = unreal.load_asset(SRC); rep['src_materials'] = [str(m.material_slot_name) for m in src.static_materials]
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'make_rawlog_75.json'), 'w'), indent=1)
