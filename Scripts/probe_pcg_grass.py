"""Dumps /Game/PerfTest/PCG_GrassScatter (nodes, settings, edges) and every PCG component in the open level:
Saved/probe_pcg_grass.json"""
import json, os
import unreal

def props(obj):
    out = {}
    for name in dir(obj):
        if name.startswith('_'):
            continue
        try:
            v = obj.get_editor_property(name)
        except Exception:
            continue
        if callable(v):
            continue
        s = str(v)
        out[name] = s[:200]
    return out

graph = unreal.load_asset('/Game/PerfTest/PCG_GrassScatter')
res = {'nodes': [], 'edges': [], 'components': []}
nodes = list(graph.get_editor_property('nodes')) + [graph.get_input_node(), graph.get_output_node()]
for n in nodes:
    s = n.get_settings()
    res['nodes'].append({'title': str(n.get_editor_property('node_title')), 'class': s.get_class().get_name() if s else None,
                         'settings': props(s) if s else {}})
    for pin in n.get_editor_property('output_pins'):
        for e in pin.get_editor_property('edges'):
            try:
                o = e.get_editor_property('output_pin'); i = e.get_editor_property('input_pin')
                res['edges'].append('{}.{} -> {}.{}'.format(o.get_editor_property('node').get_editor_property('node_title'), o.get_editor_property('properties').get_editor_property('label'),
                                                          i.get_editor_property('node').get_editor_property('node_title'), i.get_editor_property('properties').get_editor_property('label')))
            except Exception as ex:
                res['edges'].append('err ' + str(ex)[:100])
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for c in a.get_components_by_class(unreal.PCGComponent):
        g = c.get_graph()
        org, ext = a.get_actor_bounds(False)
        res['components'].append({'actor': a.get_actor_label(), 'class': a.get_class().get_name(), 'graph': g.get_path_name() if g else None,
                                  'loc': str(a.get_actor_location()), 'scale': str(a.get_actor_scale3d()), 'bounds_extent': str(ext),
                                  'generated': str(c.get_editor_property('generated_graph_output')).count('Data'),
                                  'input_type': str(c.get_editor_property('input_type')),
                                  'generation_trigger': str(c.get_editor_property('generation_trigger')),
                                  'partitioned': str(c.is_partitioned() if hasattr(c, 'is_partitioned') else '?')})
    if a.get_class().get_name() in ('PCGPartitionActor',):
        res['components'].append({'partition_actor': a.get_actor_label()})
json.dump(res, open(os.path.join(unreal.Paths.project_saved_dir(), 'probe_pcg_grass.json'), 'w'), indent=1)
