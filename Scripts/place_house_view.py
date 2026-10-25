"""Editor: puts the log house station of the last capture (Saved/bake_showcase.json, made by bake_showcase_capture.py in a
play session) into the open MechanicsTest as static pieces, folder LogHouse (an earlier copy is replaced), moved to
house_dx/dy. Saves the baked assets and the map. -> Saved/place_house_view.json"""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'place_house_view.json')
SRC = os.path.join(unreal.Paths.project_saved_dir(), 'bake_showcase.json')
MAP = '/Game/Maps/MechanicsTest'
FOLDER = 'LogHouse'
X_MIN, DX, DY = 17400.0, -16000.0, -4000.0   # station 14 starts near x=18200 (14 m per station); moved beside the log fill
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
res = {'map': world.get_outer().get_path_name()}
if res['map'] != MAP:
    res['error'] = 'open map is not MechanicsTest, nothing changed'
else:
    data = json.load(open(SRC, encoding='utf-8'))
    for a in eas.get_all_level_actors():
        if str(a.get_folder_path()).startswith(FOLDER) or a.get_actor_label() == 'TmpShowcase':
            eas.destroy_actor(a)

    def place(item, label):
        v = item['xf']
        if v[0] < X_MIN:
            return False
        mesh = unreal.load_asset(item['mesh'])
        if not mesh:
            return False
        actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(v[0] + DX, v[1] + DY, v[2]), unreal.Rotator(roll=v[3], pitch=v[4], yaw=v[5]))
        actor.set_actor_scale3d(unreal.Vector(v[6], v[7], v[8]))
        comp = actor.static_mesh_component
        comp.set_static_mesh(mesh)
        for i, m in enumerate(item.get('mats', [])):
            if m:
                comp.set_material(i, unreal.load_asset(m))
        actor.set_actor_label(label)
        actor.set_folder_path(FOLDER)
        return True
    res['logs'] = sum(place(it, 'HouseLog_%03d' % i) for i, it in enumerate(data['logs']))
    res['parts'] = sum(place(it, 'HousePart_%03d' % i) for i, it in enumerate(data['parts']))
    res['assets_saved'] = unreal.EditorAssetLibrary.save_directory('/Game/LogShowcase/Baked', only_if_is_dirty=False, recursive=True)
    res['saved'] = unreal.EditorLoadingAndSavingUtils.save_current_level()
json.dump(res, open(OUT, 'w'), indent=1)
