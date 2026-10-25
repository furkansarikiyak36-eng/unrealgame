"""KitShowcase: sets each mesh actor's real bottom (actor bounds) 2 cm above the ground, which removes z-fighting
flicker of flat pieces and drops meshes whose asset bounds misled build_kit_showcase.py (e.g. Core_Floor_Ring_2 at 18.7 m).
Also gives KiteDemo SM_AssetPlatform_02 and Skybox their kit materials (M_AssetPlatform_Grass, M_LightStage_Skybox_HDRI).
py ".../fix_showcase_placement.py"   Report: Saved/fix_showcase_placement.json (moved actors and z change)"""
import json, os
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/KitShowcase.'):
    raise RuntimeError('Open /Game/Maps/KitShowcase first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LIFT = 2.0
moved = []
for a in actors.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor) or a.get_actor_label() == 'ShowcaseGround':
        continue
    origin, ext = a.get_actor_bounds(False)
    dz = LIFT - (origin.z - ext.z)
    if abs(dz) > 0.5:
        loc = a.get_actor_location(); loc.z += dz
        a.set_actor_location(loc, False, False)
        moved.append([a.get_actor_label(), round(dz, 1)])
mats = {'/Game/KiteDemo/LevelContent/Architecture/SM_AssetPlatform_02': '/Game/KiteDemo/Environments/GroundTiles/Grass/M_AssetPlatform_Grass',
        '/Game/KiteDemo/LevelContent/Blueprints/Assets/Skybox': '/Game/KiteDemo/LevelContent/Blueprints/Assets/M_LightStage_Skybox_HDRI'}
log = os.path.join(unreal.Paths.project_saved_dir(), 'fix_checker_materials.jsonl')
for mp, matp in mats.items():
    m = unreal.load_asset(mp); mat = unreal.load_asset(matp)
    if m and mat:
        old = m.static_materials[0].material_interface
        m.set_material(0, mat); unreal.EditorAssetLibrary.save_loaded_asset(m, False)
        with open(log, 'a') as f: f.write(json.dumps([mp, 0, old.get_path_name() if old else None, matp]) + '\n')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/KitShowcase')
json.dump({'moved': len(moved), 'big': sorted(moved, key=lambda r: -abs(r[1]))[:25]},
          open(os.path.join(unreal.Paths.project_saved_dir(), 'fix_showcase_placement.json'), 'w'), indent=1)
