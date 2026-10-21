"""Read-only: instance count per mesh on GrassScatterVolume (and PCG partition actors); saves PerfTest.
py ".../count_volume_instances.py" -> Saved/count_volume_instances.json"""
import collections, json, os
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
c = collections.Counter(); comps = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == 'GrassScatterVolume' or a.get_class().get_name() == 'PCGPartitionActor':
        for comp in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            m = comp.get_editor_property('static_mesh'); comps += 1
            c[m.get_name() if m else None] += comp.get_instance_count()
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
json.dump({'components': comps, 'total': sum(c.values()), 'per_mesh': c.most_common()}, open(os.path.join(unreal.Paths.project_saved_dir(), 'count_volume_instances.json'), 'w'), indent=0)
