"""Read only: what the PCG_DenseNature volume has spawned so far. Writes Saved/pcg_status.json."""
import collections, json, os
import unreal
rep = {'components': 0, 'instances': 0, 'per_folder': {}}
per = collections.Counter()
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if 'PCGNature' in [str(t) for t in a.tags] or isinstance(a, unreal.PCGPartitionActor):
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            n = c.get_instance_count(); rep['components'] += 1; rep['instances'] += n
            m = c.static_mesh.get_path_name() if c.static_mesh else '?'
            per[m.split('/')[5] if m.count('/') > 5 else m] += n
        pc = a.get_component_by_class(unreal.PCGComponent)
        if pc: rep['generated'] = pc.get_editor_property('generated') if hasattr(pc, 'get_editor_property') else None
rep['per_folder'] = dict(per.most_common())
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'pcg_status.json'), 'w'), indent=1)
unreal.log('[PCGStatus] {} instances'.format(rep['instances']))
