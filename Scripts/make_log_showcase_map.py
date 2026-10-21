"""Editor: creates /Game/Maps/LogShowcase from the default template: sky, sun, fog; the template floor lowered 3.1 m and
enlarged as a backdrop (the diggable voxel ground is the real ground); an ALogShowcase at the origin facing +X (its
stations run along +X, 14 m apart) and a PlayerStart 9 m before it. py ".../make_log_showcase_map.py"
-> Saved/make_log_showcase_map.json. Refuses to overwrite an existing LogShowcase map unless 'force' is given."""
import json, os, sys
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'make_log_showcase_map.json')
PATH = '/Game/Maps/LogShowcase'
res = {}
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(PATH) and 'force' not in sys.argv:
    res['note'] = 'exists; opening it'
    les.load_level(PATH)
else:
    res['created'] = les.new_level_from_template(PATH, '/Engine/Maps/Templates/Template_Default')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
res['open'] = world.get_outer().get_path_name()
if res['open'] == PATH:
    actors = eas.get_all_level_actors()
    res['template'] = sorted(set(a.get_class().get_name() for a in actors))
    for a in actors:
        if isinstance(a, unreal.StaticMeshActor) and a.get_actor_location().z > -100:
            loc = a.get_actor_location()
            a.set_actor_location(unreal.Vector(0, 0, loc.z - 310.0), False, False)
            a.set_actor_scale3d(unreal.Vector(300, 300, a.get_actor_scale3d().z))
            res['floor'] = a.get_actor_label()
        if isinstance(a, unreal.PlayerStart) or isinstance(a, unreal.LogShowcase):
            eas.destroy_actor(a)
    show = eas.spawn_actor_from_class(unreal.LogShowcase, unreal.Vector(0, 0, 0), unreal.Rotator(roll=0, pitch=0, yaw=0))
    show.set_actor_label('LogShowcase')
    ps = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-900, 0, 120), unreal.Rotator(roll=0, pitch=0, yaw=0))
    res['saved'] = unreal.EditorLoadingAndSavingUtils.save_current_level()
json.dump(res, open(OUT, 'w'), indent=1, default=str)
