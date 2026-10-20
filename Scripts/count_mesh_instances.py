"""Read-only: instance count of a static mesh in the open level (all ISM/HISM components, PCG partition actors too).
py ".../count_mesh_instances.py" <mesh name> -> Saved/count_mesh_instances.json"""
import json, os, sys
import unreal
name = sys.argv[1]; inst = 0; comps = 0; shadow = set(); wpo = set(); cull = set()
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        m = c.get_editor_property('static_mesh')
        if m and m.get_name() == name:
            inst += c.get_instance_count(); comps += 1
            shadow.add(c.get_editor_property('cast_shadow')); wpo.add(c.get_editor_property('evaluate_world_position_offset'))
            cull.add((c.get_editor_property('instance_start_cull_distance'), c.get_editor_property('instance_end_cull_distance')))
json.dump({'mesh': name, 'instances': inst, 'components': comps, 'shadow': sorted(map(str, shadow)), 'wpo': sorted(map(str, wpo)),
           'cull': sorted(map(str, cull))}, open(os.path.join(unreal.Paths.project_saved_dir(), 'count_mesh_instances.json'), 'w'))
