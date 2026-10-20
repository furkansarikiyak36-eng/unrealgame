"""Read only: PCG weighted mesh entry struct. Writes Saved/probe_pcg2.json."""
import json, os
import unreal
rep = {}
e = unreal.PCGMeshSelectorWeightedEntry()
rep['empty'] = str(e.to_dict())[:1500]
txt = '(Descriptor=(StaticMesh="/Game/Fab/Megascans/Plants/Wild_Grass_vczndjqja/Low/vczndjqja_tier_3/StaticMeshes/SM_vczndjqja_VarA.SM_vczndjqja_VarA",InstanceEndCullDistance=6000,CastShadow=False),Weight=3)'
try:
    e.import_text(txt); rep['imported'] = str(e.to_dict())[:1500]; rep['export'] = e.export_text()[:800]
except Exception as ex:
    rep['import_err'] = str(ex)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
rep['world_api'] = [n for n in dir(w) if 'partition' in n.lower()]
s = unreal.PCGStaticMeshSpawnerSettings()
rep['selector_type'] = str(s.get_editor_property('mesh_selector_type'))
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'probe_pcg2.json'), 'w'), indent=1)
unreal.log('[ProbePCG2] done')
