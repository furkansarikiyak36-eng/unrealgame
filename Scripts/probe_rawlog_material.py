"""Read-only: M_RawLog texture parameters, texture sample nodes (texture each points to), MI overrides; exports candidate
wood albedos. py ".../probe_rawlog_material.py" -> Saved/probe_rawlog_material.json"""
import json, os
import unreal
MEL = unreal.MaterialEditingLibrary
out = {}
m = unreal.load_asset('/Game/UnrealExport/Materials/M_RawLog')
out['params'] = [str(n) for n in MEL.get_texture_parameter_names(m)]
nodes = []
for cls in (unreal.MaterialExpressionTextureSampleParameter2D, unreal.MaterialExpressionTextureSample):
    pass
try:
    exprs = m.get_editor_property('expression_collection').get_editor_property('expressions')
except Exception as e:
    exprs = []; out['expr_err'] = str(e)[:200]
for ex in exprs:
    if isinstance(ex, unreal.MaterialExpressionTextureBase):
        t = ex.get_editor_property('texture')
        row = {'node': ex.get_name(), 'class': ex.get_class().get_name(), 'texture': t.get_path_name() if t else None,
               'sampler': str(ex.get_editor_property('sampler_type'))}
        try: row['param'] = str(ex.get_editor_property('parameter_name'))
        except Exception: pass
        nodes.append(row)
out['nodes'] = nodes
out['mi'] = {}
for n in ('Cut', 'Hewn', 'Peeled', 'PeeledCut', 'Bark'):
    mi = unreal.load_asset('/Game/UnrealExport/Materials/MI_RawLog_' + n)
    if mi:
        out['mi'][n] = {'parent': mi.get_editor_property('parent').get_path_name(),
                        'tex': {str(p.parameter_info.name): (p.parameter_value.get_path_name() if p.parameter_value else None)
                                for p in mi.get_editor_property('texture_parameter_values')},
                        'scalar': {str(p.parameter_info.name): p.parameter_value for p in mi.get_editor_property('scalar_parameter_values')},
                        'vector': {str(p.parameter_info.name): str(p.parameter_value) for p in mi.get_editor_property('vector_parameter_values')}}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'probe_rawlog_material.json'), 'w'), indent=1)
