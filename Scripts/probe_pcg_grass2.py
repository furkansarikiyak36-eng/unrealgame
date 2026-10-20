"""PCG grass output in the open level: instance counts per mesh on GrassScatterVolume, spawner entries.
Saved/probe_pcg_grass2.json"""
import json, os
import unreal
res = {'components': [], 'entries': []}
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == 'GrassScatterVolume' or a.get_class().get_name() == 'PCGPartitionActor':
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            m = c.get_editor_property('static_mesh')
            b = m.get_bounding_box() if m else None
            res['components'].append({'actor': a.get_actor_label(), 'mesh': m.get_path_name() if m else None,
                                      'mesh_h_cm': round(b.max.z - b.min.z) if b else None,
                                      'instances': c.get_instance_count(), 'cast_shadow': c.get_editor_property('cast_shadow'),
                                      'cull': [c.get_editor_property('instance_start_cull_distance'), c.get_editor_property('instance_end_cull_distance')],
                                      'collision': str(c.get_collision_enabled()),
                                      'first_z': c.get_instance_transform(0, True).translation.z if c.get_instance_count() else None})
g = unreal.load_asset('/Game/PerfTest/PCG_GrassScatter')
for n in g.get_editor_property('nodes'):
    s = n.get_settings()
    if isinstance(s, unreal.PCGStaticMeshSpawnerSettings):
        for e in s.get_editor_property('mesh_selector_parameters').get_editor_property('mesh_entries'):
            res['entries'].append(e.export_text()[:400])
json.dump(res, open(os.path.join(unreal.Paths.project_saved_dir(), 'probe_pcg_grass2.json'), 'w'), indent=1)
