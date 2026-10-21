"""Read only: exposed parameters of a few materials. Writes Saved/probe_material_params.json."""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'probe_material_params.json')
lib = unreal.MaterialEditingLibrary
rep = {}
for p in ['/Game/Fab/Materials/Standard/M_MS_Srf', '/Game/Fab/Materials/Standard/M_MS_Base',
          '/ProceduralVegetationEditor/SampleAssets/Materials/MasterMaterials/MA_Foliage_Trees_TwoBarkBlend',
          '/Game/Building/Materials/M_CutWood']:
    m = unreal.load_asset(p)
    if not m:
        rep[p] = 'missing'; continue
    rep[p] = {'scalar': [str(x) for x in lib.get_scalar_parameter_names(m)],
              'vector': [str(x) for x in lib.get_vector_parameter_names(m)],
              'texture': [str(x) for x in lib.get_texture_parameter_names(m)],
              'static': [str(x) for x in lib.get_static_switch_parameter_names(m)]}
    try:
        rep[p]['domain'] = str(m.get_editor_property('material_domain')); rep[p]['blend'] = str(m.get_editor_property('blend_mode'))
        rep[p]['expressions'] = lib.get_num_material_expressions(m)
    except Exception as e:
        rep[p]['err'] = str(e)[:100]
t = unreal.load_asset('/Game/Megaplant_Library/Tree_Baltic_Pine/Textures/T_Baltic_PineBark_01_NAH')
rep['NAH'] = {'vt': t.get_editor_property('virtual_texture_streaming'), 'srgb': t.get_editor_property('srgb'), 'comp': str(t.get_editor_property('compression_settings'))}
json.dump(rep, open(OUT, 'w'), indent=1)
unreal.log('[ProbeMat] done')
