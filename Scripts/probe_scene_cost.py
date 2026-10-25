"""Everything drawn or ticking in the open level, for the cost report: Saved/scene_cost.json
- instanced meshes: per mesh instances, Nanite, LOD count, triangles per LOD, shadow, cull distances
- static and skeletal mesh actors: mesh, triangles (LOD0 / Nanite fallback), shadow
- actor classes and how many tick"""
import collections, json, os
import unreal
def tris(m):
    out = []
    for i in range(m.get_num_lods()):
        try:
            out.append(m.get_num_triangles(i))
        except Exception:
            out.append(None)
    return out
inst = {}
classes = collections.Counter(); ticking = collections.Counter(); skel = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    cn = a.get_class().get_name(); classes[cn] += 1
    if a.is_actor_tick_enabled() if hasattr(a, 'is_actor_tick_enabled') else False:
        ticking[cn] += 1
    for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        m = c.get_editor_property('static_mesh')
        if not m:
            continue
        k = m.get_path_name()
        e = inst.setdefault(k, {'mesh': m.get_name(), 'folder': k.split('/')[-3] if k.count('/') > 3 else '', 'owner': a.get_actor_label(),
                                'instances': 0, 'nanite': m.get_editor_property('nanite_settings').enabled, 'tris_per_lod': tris(m),
                                'shadow': c.get_editor_property('cast_shadow'),
                                'cull_end_cm': c.get_editor_property('instance_end_cull_distance')})
        e['instances'] += c.get_instance_count()
    if isinstance(a, unreal.SkeletalMeshActor):
        sk = a.skeletal_mesh_component.get_editor_property('skeletal_mesh_asset')
        b = sk.get_bounds() if sk else None
        skel.append({'label': a.get_actor_label(), 'mesh': sk.get_name() if sk else None, 'tags': [str(t) for t in a.tags],
                     'height_m': round((b.origin.z + b.box_extent.z) / 100, 1) if b else None,
                     'shadow': a.skeletal_mesh_component.cast_shadow})
json.dump({'instanced': sorted(inst.values(), key=lambda e: -e['instances']), 'skeletal': skel,
           'classes': dict(classes.most_common()), 'ticking_actors': dict(ticking.most_common())},
          open(os.path.join(unreal.Paths.project_saved_dir(), 'scene_cost.json'), 'w'), indent=1)
