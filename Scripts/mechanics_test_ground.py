"""Editor: MechanicsTest ground = the template floor, top at z=0, solid (BlockAll), tagged NoDiggableGround so the
voxel ground does not spawn on play. Marks the actor modified so the save really writes it, then reloads the map and
reads it back. -> Saved/mechanics_ground.json"""
import json, os
import unreal
PATH = '/Game/Maps/MechanicsTest'
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(PATH)
res = {}
for a in eas.get_all_level_actors():
    if a.get_actor_label() == 'Floor':
        a.modify(True)
        o, e = a.get_actor_bounds(False)
        loc = a.get_actor_location()
        a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z - (o.z + e.z)), False, False)
        c = a.get_component_by_class(unreal.StaticMeshComponent)
        c.modify(True)
        c.set_collision_profile_name('BlockAll')
        a.tags = [unreal.Name('NoDiggableGround')]
res['saved'] = unreal.EditorLoadingAndSavingUtils.save_current_level()
les.load_level(PATH)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == 'Floor':
        o, e = a.get_actor_bounds(False)
        res['after_reload'] = {'tags': [str(t) for t in a.tags], 'top': o.z + e.z, 'coll': str(a.get_component_by_class(unreal.StaticMeshComponent).get_collision_profile_name())}
json.dump(res, open(os.path.join(unreal.Paths.project_saved_dir(), 'mechanics_ground.json'), 'w'))
