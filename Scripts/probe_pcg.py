"""Read only: PCG Python API names. Writes Saved/probe_pcg.json."""
import json, os
import unreal
rep = {}
def names(o, keys=None):
    return sorted(n for n in dir(o) if not n.startswith('_') and (not keys or any(k in n.lower() for k in keys)))
rep['graph'] = names(unreal.PCGGraph, ['node', 'edge', 'input', 'output', 'add', 'param'])
rep['spawner'] = names(unreal.PCGStaticMeshSpawnerSettings)
rep['weighted'] = names(unreal.PCGMeshSelectorWeighted)
rep['entry'] = names(unreal.PCGMeshSelectorWeightedEntry)
rep['descriptor'] = names(unreal.SoftISMComponentDescriptor, ['mesh', 'cull', 'shadow', 'collision'])
rep['component'] = names(unreal.PCGComponent, ['gen', 'graph', 'partition', 'seed', 'clean', 'trigger', 'runtime'])
rep['volume'] = names(unreal.PCGVolume)
rep['volsampler'] = names(unreal.PCGVolumeSamplerSettings)
rep['noise'] = names(unreal.PCGSpatialNoiseSettings)
rep['transform'] = names(unreal.PCGTransformPointsSettings)
rep['classes'] = sorted(n for n in dir(unreal) if n.startswith('PCG') and ('Surface' in n or 'Sampler' in n or 'Selector' in n or 'Partition' in n or 'World' in n))[:60]
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
rep['world_partition'] = bool(w.get_world_partition()) if hasattr(w, 'get_world_partition') else 'n/a'
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'probe_pcg.json'), 'w'), indent=1)
unreal.log('[ProbePCG] done')
